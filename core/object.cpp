#include "hash.h"
#include "object.h"
#include "expression.h"
#include "gc/collect.h"
#include "native/native.h"

extern int integer_native_method_table_count;
extern InkNative_MethodTable integer_native_method_table[];
extern int string_native_method_table_count;
extern InkNative_MethodTable string_native_method_table[];
extern int object_native_method_table_count;
extern InkNative_MethodTable object_native_method_table[];
extern int array_native_method_table_count;
extern InkNative_MethodTable array_native_method_table[];
extern int function_native_method_table_count;
extern InkNative_MethodTable function_native_method_table[];

Ink_Object *getMethod(const char *name, InkNative_MethodTable *table, int count)
{
	int i;
	for (i = 0; i < count; i++) {
		if (!strcmp(name, table[i].name)) {
			return new Ink_FunctionObject(as<Ink_FunctionObject>(table[i].func)->native);
		}
	}
	return NULL;
}

Ink_Object *Ink_Object::getSlot(const char *key)
{
	Ink_HashTable *ret = getSlotMapping(key);

	return ret ? ret->value : new Ink_Undefined();
}

Ink_HashTable *Ink_Object::getSlotMapping(const char *key)
{
	Ink_HashTable *i;
	Ink_HashTable *ret = NULL;
	Ink_Object *method = NULL;

	for (i = hash_table; i; i = i->next) {
		if (!strcmp(i->key, key)){
			for (ret = i; ret->bonding; ret = ret->bonding) ;
			ret->bondee = i;
			return ret;
		}
	}

#if 1
	switch (type) {
		case INK_INTEGER:
			if (method = getMethod(key, integer_native_method_table,
								   integer_native_method_table_count)) {
				ret = setSlot(key, method, false);
			}
			break;
		case INK_STRING:
			if (method = getMethod(key, string_native_method_table,
								   string_native_method_table_count)) {
				ret = setSlot(key, method, false);
			}
			break;
		case INK_ARRAY:
			if (method = getMethod(key, array_native_method_table,
								   array_native_method_table_count)) {
				ret = setSlot(key, method, false);
			}
			break;
		case INK_FUNCTION:
			if (method = getMethod(key, function_native_method_table,
								   function_native_method_table_count)) {
				ret = setSlot(key, method, false);
			}
			break;
		default: break;
	}
	if (!ret && (method = getMethod(key, object_native_method_table,
									object_native_method_table_count))) {
		ret = setSlot(key, method, false);
	}
#endif

	return ret;
}

Ink_HashTable *Ink_Object::setSlot(const char *key, Ink_Object *value, bool if_check_exist)
{
	Ink_HashTable *slot = NULL;

	if (if_check_exist) slot = getSlotMapping(key);

	if (slot) {
		slot->value = value;
	} else {
		slot = new Ink_HashTable(key, value);
		if (hash_table)
			hash_table->getEnd()->next = slot;
		else
			hash_table = slot;
	}

	return slot;
}

void Ink_Object::deleteSlot(const char *key)
{
	Ink_HashTable *i, *prev;

	for (i = hash_table, prev = NULL; i; prev = i, i = i->next) {
		if (!strcmp(i->key, key)) {
			if (prev) {
				prev->next = i->next;
				delete i;
				return;
			} else {
				hash_table = i->next;
				delete i;
			}
		}
	}

	return;
}

void Ink_Object::cleanHashTable()
{
	cleanHashTable(hash_table);
	hash_table = NULL;

	return;
}

void Ink_Object::cleanHashTable(Ink_HashTable *table)
{
	Ink_HashTable *i, *tmp;
	for (i = table; i;) {
		tmp = i;
		i = i->next;
		delete tmp;
	}

	return;
}

void Ink_Object::cloneHashTable(Ink_Object *src, Ink_Object *dest)
{
	Ink_HashTable *i;
	for (i = src->hash_table; i; i = i->next) {
		dest->setSlot(i->key, i->value);
	}

	return;
}

Ink_Object *Ink_Object::clone()
{
	Ink_Object *new_obj = new Ink_Object();

	cloneHashTable(this, new_obj);

	return new_obj;
}

Ink_Object *Ink_Integer::clone()
{
	Ink_Object *new_obj = new Ink_Integer(value);

	cloneHashTable(this, new_obj);

	return new_obj;
}

Ink_Object *Ink_String::clone()
{
	Ink_Object *new_obj = new Ink_String(value);

	cloneHashTable(this, new_obj);

	return new_obj;
}

extern IGC_CollectEngine *global_engine;
extern IGC_CollectEngine *current_engine;

Ink_Object *Ink_FunctionObject::call(Ink_ContextChain *context, unsigned int argc, Ink_Object **argv, bool return_this)
{
	unsigned int argi, j;
	// Ink_HashTable *i;
	Ink_ContextObject *local; // new local context
	Ink_Object *ret_val = NULL;
	IGC_CollectEngine *engine_backup = current_engine;

	IGC_CollectEngine *gc_engine = new IGC_CollectEngine();
	IGC_initGC(gc_engine);

	local = new Ink_ContextObject();
	if (closure_context) context = closure_context->copyContextChain();
	if (!is_inline) { // if not inline function, set local context
		local->setSlot("base", getSlot("base"));
		local->setSlot("this", this);
	}
	context->addContext(local);

	gc_engine->initContext(context);

	if (is_native) ret_val = native(context, argc, argv);
	else {
		for (j = 0, argi = 0; j < param.size() && argi < argc; j++, argi++) {
			local->setSlot(param[j]->c_str(), argv[argi]); // initiate local argument
		}

		if (j < param.size() || argi < argc) {
			InkWarn_Unfit_Argument();
		}

		for (j = 0; j < exp_list.size(); j++) {
			gc_engine->checkGC();
			ret_val = exp_list[j]->eval(context); // eval each expression
			if (CGC_if_return) {
				if (!is_inline)
					CGC_if_return = false;
				break;
			}
		}
		if (return_this) {
			ret_val = local->getSlot("this");
		}
	}

	//gc_engine->collectGarbage();
	context->removeLast();
	
	if (ret_val)
		gc_engine->doMark(ret_val);
	gc_engine->collectGarbage();

	if (closure_context) Ink_ContextChain::disposeContextChain(context);

	//gc_engine->doMark(ret_val);
	//gc_engine->collectGarbage();

	//delete gc_engine;
	if (engine_backup) engine_backup->link(gc_engine);
	current_engine = engine_backup;
	delete gc_engine;

	return ret_val ? ret_val : new Ink_NullObject(); // return the last expression
}

Ink_FunctionObject::~Ink_FunctionObject()
{
	if (closure_context) Ink_ContextChain::disposeContextChain(closure_context);
	cleanHashTable();
}

Ink_ArrayValue Ink_Array::cloneArrayValue(Ink_ArrayValue val)
{
	Ink_ArrayValue ret = Ink_ArrayValue();
	unsigned int i;

	for (i = 0; i < val.size(); i++) {
		if (val[i])
			ret.push_back(new Ink_HashTable("", val[i]->value));
	}

	return ret;
}

Ink_Object *Ink_Array::clone()
{
	Ink_Object *new_obj = new Ink_Array(cloneArrayValue(value));

	cloneHashTable(this, new_obj);

	return new_obj;
}

Ink_Object *Ink_FunctionObject::clone()
{
	Ink_FunctionObject *new_obj = new Ink_FunctionObject();

	new_obj->is_native = is_native;
	new_obj->is_inline = is_inline;
	new_obj->native = native;

	new_obj->param = param;
	new_obj->exp_list = exp_list;
	new_obj->closure_context = closure_context->copyContextChain();

	cloneHashTable(this, new_obj);

	return new_obj;
}