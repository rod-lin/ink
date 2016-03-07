#ifndef _CORE_GENERAL_H_
#define _CORE_GENERAL_H_

#include <vector>
#include <string>
#include <stdlib.h>
#include "inttype.h"
#include "../includes/universal.h"

#define UNDEFINED (new Ink_Undefined(engine))
#define NULL_OBJ (new Ink_NullObject(engine))
#define TRUE_OBJ (new Ink_Numeric(engine, 1))

#define RETURN_FLAG (engine->CGC_interrupt_signal == INTER_RETURN)
#define BREAK_FLAG (engine->CGC_interrupt_signal == INTER_BREAK)
#define CONTINUE_FLAG (engine->CGC_interrupt_signal == INTER_CONTINUE)
#define DROP_FLAG (engine->CGC_interrupt_signal == INTER_DROP)

#define DEFAULT_SIGNAL (INTER_RETURN | INTER_BREAK | INTER_CONTINUE | INTER_DROP)
#define ALL_SIGNAL (INTER_RETURN | INTER_BREAK | \
					INTER_CONTINUE | INTER_DROP | \
					INTER_THROW | INTER_RETRY | INTER_EXIT)

namespace ink {

inline int removeDir(const std::string path, bool if_delete_sub = true);
inline char *getCurrentDir();
inline int changeDir(const char *path);

}

#if defined(INK_PLATFORM_LINUX)
#include <unistd.h>

namespace ink {

inline int removeDir(const std::string path, bool if_delete_sub)
{
	return system(("rm -r \"" + path + "\"").c_str());
}

inline char *getCurrentDir()
{
	return getcwd(NULL, 0);
}

inline int changeDir(const char *path)
{
	return chdir(path);
}

}
#elif defined(INK_PLATFORM_WIN32)
#include <stdio.h>
#include <windows.h>
#include <conio.h>
#include <direct.h>

namespace ink {

inline int removeDir(const std::string path, bool if_delete_sub)
{
	bool has_sub_dir = false;
	HANDLE file_handle;
	std::string tmp_file_path;
	std::string pattern;
	WIN32_FIND_DATA file_info;

	pattern = path + "\\*";
	file_handle = FindFirstFile(pattern.c_str(), &file_info);
	if (file_handle != INVALID_HANDLE_VALUE) {
		do {
			if (file_info.cFileName[0] != '.') {
				tmp_file_path.erase();
				tmp_file_path = path + "\\" + file_info.cFileName;

				if (file_info.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) {
					if (if_delete_sub) {
						int i = removeDir(tmp_file_path, if_delete_sub);
						if (i) return i;
					} else has_sub_dir = true;
				} else {
				  if (SetFileAttributes(tmp_file_path.c_str(),
				                         FILE_ATTRIBUTE_NORMAL) == false)
				    return GetLastError();

				  if (DeleteFile(tmp_file_path.c_str()) == false)
				    return GetLastError();
				}
			}
		} while (FindNextFile(file_handle, &file_info) == true);

		FindClose(file_handle);

		DWORD dwError = GetLastError();
		if (dwError != ERROR_NO_MORE_FILES)
			return dwError;
		else {
			if (!has_sub_dir) {
				if (SetFileAttributes(path.c_str(),
									   FILE_ATTRIBUTE_NORMAL) == false)
				return GetLastError();

				if (RemoveDirectory(path.c_str()) == false)
					return GetLastError();
			}
		}
	}

	return 0;
}

inline char *getCurrentDir()
{
	return _getcwd(NULL, 0);
}

inline int changeDir(const char *path)
{
	return _chdir(path);
}

}
#endif

namespace ink {

typedef Ink_UInt64 Ink_InterruptSignal;
#define INTER_NONE			((Ink_InterruptSignal)INTER_NONE_tag)
#define INTER_RETURN		((Ink_InterruptSignal)INTER_RETURN_tag)
#define INTER_BREAK			((Ink_InterruptSignal)INTER_BREAK_tag)
#define INTER_CONTINUE		((Ink_InterruptSignal)INTER_CONTINUE_tag)
#define INTER_DROP			((Ink_InterruptSignal)INTER_DROP_tag)
#define INTER_THROW			((Ink_InterruptSignal)INTER_THROW_tag)
#define INTER_RETRY			((Ink_InterruptSignal)INTER_RETRY_tag)
#define INTER_EXIT			((Ink_InterruptSignal)INTER_EXIT_tag)
#define INTER_LAST			((Ink_InterruptSignal)INTER_LAST_tag)

typedef enum {
	INTER_NONE_tag 		= 1 << 1,
	INTER_RETURN_tag 	= 1 << 2,
	INTER_BREAK_tag 	= 1 << 3,
	INTER_CONTINUE_tag 	= 1 << 4,
	INTER_DROP_tag 		= 1 << 5,
	INTER_THROW_tag 	= 1 << 6,
	INTER_RETRY_tag 	= 1 << 7,
	INTER_EXIT_tag 		= 1 << 8,
	INTER_LAST_tag 		= 1 << 16
} Ink_InterruptSignal_tag;

class Ink_Expression;
class Ink_Argument;
class Ink_Object;
class Ink_InterpreteEngine;
class Ink_HashTable;
class Ink_Undefined;
class Ink_NullObject;

typedef Ink_InterruptSignal Ink_InterruptSignalTrap;
typedef std::vector<Ink_Expression *> Ink_ExpressionList;
typedef std::vector<Ink_Argument *> Ink_ArgumentList;

typedef Ink_UInt32 Ink_ArgcType;
typedef Ink_SInt32 IGC_MarkType;
typedef Ink_UInt64 IGC_ObjectCountType;
typedef Ink_SInt64 Ink_LineNoType;
typedef Ink_SInt32 Ink_ExceptionCode;
typedef double Ink_NumericValue;
typedef std::vector<Ink_HashTable *> Ink_ArrayValue;

typedef void (*IGC_Marker)(Ink_InterpreteEngine *engine, Ink_Object *obj);
typedef std::pair<Ink_HashTable *, Ink_HashTable *> IGC_Bonding;
typedef std::vector<IGC_Bonding> IGC_BondingList;

extern Ink_Undefined *ink_global_constant_undefined;
extern Ink_NullObject *ink_global_constant_null;

template <typename T1, typename T2, typename T3>
class triple {
public:
	T1 first;
	T2 second;
	T3 third;

	triple(T1 first)
	: first(first)
	{ }

	triple(T1 first, T2 second)
	: first(first), second(second)
	{ }

	triple(T1 first, T2 second, T3 third)
	: first(first), second(second), third(third)
	{ }
};

class Ink_Argument {
public:
	Ink_Expression *arg;

	bool is_expand;
	Ink_Expression *expandee;

	Ink_Argument(Ink_Expression *arg)
	: arg(arg), is_expand(false), expandee(NULL)
	{ }

	Ink_Argument(bool is_expand, Ink_Expression *expandee)
	: arg(NULL), is_expand(is_expand), expandee(expandee)
	{ }

	~Ink_Argument();
};

class Ink_Parameter {
public:
	std::string *name;
	bool is_ref;
	bool is_variant;
	bool is_optional;

	Ink_Parameter(std::string *name)
	: name(name), is_ref(false), is_variant(false), is_optional(true)
	{ }

	Ink_Parameter(std::string *name, bool is_ref)
	: name(name), is_ref(is_ref), is_variant(false), is_optional(true)
	{ }

	Ink_Parameter(std::string *name, bool is_ref, bool is_variant)
	: name(name), is_ref(is_ref), is_variant(is_variant), is_optional(true)
	{ }

	Ink_Parameter(std::string *name, bool is_ref, bool is_variant, bool is_optional)
	: name(name), is_ref(is_ref), is_variant(is_variant), is_optional(is_optional)
	{ }
};
typedef std::vector<Ink_Parameter> Ink_ParamList;

inline bool hasSignal(Ink_InterruptSignalTrap set, Ink_InterruptSignal sig)
{
	return (~(~set | sig) != set) && sig < INTER_LAST;
}

inline Ink_InterruptSignalTrap addSignal(Ink_InterruptSignalTrap set, Ink_InterruptSignal sign)
{
	return set | sign;
}

class Ink_FunctionAttribution {
public:
	Ink_InterruptSignalTrap interrupt_signal_trap;
	
	Ink_FunctionAttribution()
	: interrupt_signal_trap(DEFAULT_SIGNAL)
	{ }

	Ink_FunctionAttribution(Ink_InterruptSignalTrap trap)
	: interrupt_signal_trap(trap)
	{ }

	inline bool hasTrap(Ink_InterruptSignal sig)
	{
		return hasSignal(interrupt_signal_trap, sig);
	}
};

void Ink_initEnv();
void Ink_removeTmpDir();
void Ink_disposeEnv();

template <class T> T *as(Ink_Expression *obj)
{
	return dynamic_cast<T*>(obj);
}

template <class T> T *as(Ink_Object *obj)
{
	return dynamic_cast<T *>(obj);
}

}

#endif
