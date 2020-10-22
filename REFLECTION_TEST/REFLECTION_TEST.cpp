//
//This is a complete implementation of the use of reflection for serialization/deserialization operations in a neat, end user editable way
//It should compile with any version of C++, or C with minor modifications
//The big benefit I see from this approach is that there are no extra clases nor any extra state that needs to be created on runtime
//

//#undef UNICODE /*quickily test for ansi*/

#include <Windows.h>//non windows users I'm sorry, you got a little porting work to do
#include <string> //yes, nasty
#include <Shlobj.h> //SHGetKnownFolderPath, just for completeness sake

typedef int i32;
typedef long long i64;
typedef unsigned int u32;
typedef float f32;
typedef double f64;
#ifdef UNICODE
typedef std::wstring str;
#else
typedef std::string str;
#endif

#ifdef UNICODE
#define to_str(v) std::to_wstring(v)
#else
#define to_str(v) std::to_string(v)
#endif

bool str_found(size_t p) {
	return p != str::npos;
}

//NOTE: I'm using str because of the ease of use around the macros, but actually every separator-like macro MUST be only 1 character long, not all functions were made with more than 1 character in mind, though most are

i32 n_tabs;//TODO: use n_tabs for deserialization also, it would help to know how many tabs should be before an identifier to do further filtering //NOTE:in a big project this variable will need to be declared as extern

#define _BeginSerialize() n_tabs=0
#define _BeginDeserialize() n_tabs=0
#define _AddTab() n_tabs++
#define _RemoveTab() n_tabs--
#define _generate_member(type,name,...) type name {__VA_ARGS__};
#define _keyvaluesepartor str(TEXT("="))
#define _structbegin str(TEXT("{"))
#define _structend str(TEXT("}"))
#define _memberseparator str(TEXT(","))
#define _newline str(TEXT("\n"))
#define _tab TEXT('\t')
#define _tabs str(n_tabs,_tab)
#define _serialize_member(type,name,...) + _tabs + str(TEXT(#name)) + _keyvaluesepartor + serial::serialize(name) + _newline /*TODO:find a way to remove this last +*/
#define _serialize_struct(var) str(TEXT(#var)) + _keyvaluesepartor + (var).serialize() + _newline
#define _deserialize_struct(var,serialized_content) (var).deserialize(str(TEXT(#var)),serialized_content);
#define _deserialize_member(type,name,...) serial::deserialize(name,str(TEXT(#name)),substr); /*NOTE: requires for the variable: str substr; to exist and contain the string for deserialization*/

//NOTE: offset should be 1 after the last character of "begin"
//NOTE: returns str::npos if not found
size_t find_closing_str(str text, size_t offset, str begin, str close) {
	//Finds closing str, eg: open= { ; close= } ; you're here-> {......{..}....} <-finds this one
	size_t closePos = offset-1;
	i32 counter = 1;
	while (counter > 0 && text.size() > ++closePos) {
		if (!text.compare(closePos, begin.size(), begin)) {
			counter++;
		}
		else if (!text.compare(closePos, close.size(), close)) {
			counter--;
		}
	}
	return !counter?closePos:str::npos;
}

//NOTE: offset corresponds to "s"
//the char behind an identifier should only be one of this: "begin of file", _tab, _newline, _memberseparator, _structbegin
size_t find_identifier(str s, size_t offset, str compare) {
	for (size_t p; str_found( p=s.find(compare, offset) );) {
		if (p == 0) return p;
		if (s[p - 1] == _tab) return p;
		if (s[p - 1] == _newline[0]) return p;
		if (s[p - 1] == _memberseparator[0]) return p;
		if (s[p - 1] == _structbegin[0]) return p;
		offset = p + 1;
	}
	return str::npos;
}

//Returns first non matching char, could be 1 past the size of the string
size_t find_till_no_match(str s, size_t offset, str match) {
	size_t p = offset - 1;
	while (s.size() > ++p) {
		size_t r = (str(1,s[p])).find_first_of(match.c_str());//NOTE: find_first_of is really badly designed
		if (!str_found(r)) {
			break;
		}
	}
	return p;
}

COLORREF ColorFromBrush(HBRUSH br) {
	LOGBRUSH lb;
	GetObject(br, sizeof(lb), &lb);
	return lb.lbColor;
}

//Sending everyone into a namespace avoids having to include a multitude of files in the soon to be serialization.h since you can re-open the namespace from other files
namespace serial {
	str serialize(i32 var) {//Basic types are done by hand since they are the building blocks
		str res = to_str(var);
		return res;
	}
	str serialize(f32 var) {
		str res = to_str(var);
		return res;
	}

	str serialize(RECT var) {//Also simple structs that we know wont change
		str res = _structbegin + str(TEXT("left")) + _keyvaluesepartor + serialize(var.left) + _memberseparator + str(TEXT("top")) + _keyvaluesepartor + serialize(var.top) + _memberseparator + str(TEXT("right")) + _keyvaluesepartor + serialize(var.right) + _memberseparator + str(TEXT("bottom")) + _keyvaluesepartor + serialize(var.bottom) + _structend;
		return res;
	}

	str serialize(HBRUSH var) {//You can store not the pointer but some of its contents
		COLORREF col = ColorFromBrush(var);
		str res = _structbegin + str(TEXT("R")) + _keyvaluesepartor + serialize(GetRValue(col)) + _memberseparator + str(TEXT("G")) + _keyvaluesepartor + serialize(GetGValue(col)) + _memberseparator + str(TEXT("B")) + _keyvaluesepartor + serialize(GetBValue(col)) + _structend;
		return res;
	}

	bool deserialize(i32& var, str name, const str& content) {//We use references in deserialization to simplify the macros
		str start = name + _keyvaluesepartor;
		str numeric_i = str(TEXT("1234567890.-"));
		size_t s = find_identifier(content,0,start);
		size_t e = find_till_no_match(content,s+start.size(),numeric_i);
		if (str_found(s) && str_found(e)) {
			s += start.size();
			str substr = content.substr(s, e - s);
			try {
				i32 temp = std::stoi(substr);
				var = temp;
				return true;
			}
			catch (...) {}
		}
		return false;
	}
	
	bool deserialize(f32& var, str name, const str& content) {
		str start = name + _keyvaluesepartor;
		str numeric_f = str(TEXT("1234567890.-"));
		size_t s = find_identifier(content,0,start);
		size_t e = find_till_no_match(content, s + start.size(), numeric_f);
		if (str_found(s) && str_found(e)) {
			s += start.size();
			str substr = content.substr(s, e - s);
			try {
				f32 temp = std::stof(substr);//TODO: check whether stof is locale dependent
				var = temp;
				return true;
			}
			catch (...) {}
		}
		return false;
	}

	bool deserialize(HBRUSH& var, str name, const str& content) {
		str start = name + _keyvaluesepartor + _structbegin;
		size_t s = find_identifier(content,0,start);
		size_t e = find_closing_str(content, s + start.size(), _structbegin, _structend);
		if (str_found(s) && str_found(e)) {
			s += start.size();
			str substr = content.substr(s, e - s);
			i32 R, G, B;
			bool r1 =_deserialize_member(0, R, 0);
			bool r2 =_deserialize_member(0, G, 0);
			bool r3 =_deserialize_member(0, B, 0);
			if (r1&&r2&&r3) {
				COLORREF col = RGB(R,G,B);
				var = CreateSolidBrush(col);
				return true;
			}
		}
		return false;
	}

	bool deserialize(RECT& var, str name, const str& content) {
		str start = name + _keyvaluesepartor + _structbegin;
		size_t s = find_identifier(content, 0,start);
		size_t e = find_closing_str(content, s+start.size(), _structbegin, _structend);
		if (str_found(s) && str_found(e)) {
			s += start.size();
			str substr = content.substr(s, e - s);
			i32 left, top, right,bottom;
			bool r1 = _deserialize_member(0, left, 0);
			bool r2 = _deserialize_member(0, top, 0);
			bool r3 = _deserialize_member(0, right, 0);
			bool r4 = _deserialize_member(0, bottom, 0);
			if (r1&&r2&&r3&&r4) {
				var = { left,top,right,bottom };
				return true;
			}
		}
		return false;
	}

}

struct nc{
	//The magic begins here: we create our structs in a separate format to allow for a user defined function to operate over them
	#define foreach_nc_member(op) \
		op(i32,a,5) \
		op(HBRUSH,br,CreateSolidBrush(RGB(123,48,68))) \

	foreach_nc_member(_generate_member);

	//Here comes a tiny bit of a limitation with the implementation, we need for the higher level structs to also have serialize/deserialize as member functions, if you dont want to you could create a super struct with all the other structs you want to save/load inside, and hardcode the initial serialization/deserialization step to enter the string-ed name of that struct
	//Also if you wanted to use classes with private members you'll most probably have to use this method
	str serialize() {
		_AddTab();
		str res = _structbegin +_newline foreach_nc_member(_serialize_member);
		_RemoveTab();
		res += _tabs + _structend;
		return res;
	}

	bool deserialize(str name, const str& content) {
		str start = name + _keyvaluesepartor + _structbegin + _newline;
		size_t s = find_identifier(content,0,start);
		size_t e = find_closing_str(content,s + start.size(), _structbegin,_structend);
		if (str_found(s) && str_found(e)) {
			s += start.size();
			str substr = content.substr(s, e - s);
			foreach_nc_member(_deserialize_member);
			return true;
		}
		return false;
	}
};
namespace serial {
	str serialize(nc var) {
		return var.serialize();
	}
	bool deserialize(nc& var, str name, const str& content) {
		return var.deserialize(name, content);
	}
}
struct cl{
#define foreach_cl_member(op) \
		op(RECT,b, 23,24,25,26) \
		op(f32,f,1.4575724f) \
		op(nc,n,) \

	foreach_cl_member(_generate_member);

	str serialize() {
		_AddTab();
		str res = _structbegin + _newline foreach_cl_member(_serialize_member);
		_RemoveTab();
		res += _tabs + _structend;
		return res;
	}

	bool deserialize(str name, const str& content) {
		str start = name + _keyvaluesepartor + _structbegin + _newline;
		size_t s = find_identifier(content, 0,start);
		size_t e = find_closing_str(content, s + start.size(), _structbegin, _structend);
		if (str_found(s) && str_found(e)) {
			s += start.size();
			str substr = content.substr(s, e - s);
			foreach_cl_member(_deserialize_member);
			return true;
		}
		return false;
	}

};
//TODO(for the reader): add "cl" to the serial namespace

str load_file() {
	PWSTR folder;
	SHGetKnownFolderPath(FOLDERID_Desktop, 0, NULL, &folder);//NOTE: I dont think this has an ansi version that isnt deprecated
	std::wstring file = folder + std::wstring(L"\\serialized.txt");
	CoTaskMemFree(folder);

	str res;
	HANDLE hFile = CreateFileW(file.c_str(), GENERIC_READ, FILE_SHARE_DELETE | FILE_SHARE_READ | FILE_SHARE_WRITE, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);

	if (hFile != INVALID_HANDLE_VALUE) {
		LARGE_INTEGER sz;
		if (GetFileSizeEx(hFile, &sz)) {
			u32 sz32 = (u32)sz.QuadPart;
			TCHAR* buf = (TCHAR*)malloc(sz32);
			if (buf) {
				DWORD bytes_read;
				if (ReadFile(hFile, buf, sz32, &bytes_read, 0) && sz32 == bytes_read) {
					res = buf;
				}
				free(buf);
			}
		}
		CloseHandle(hFile);
	}
	return res;
}

void save_to_file(str content) {
	PWSTR folder;
	SHGetKnownFolderPath(FOLDERID_Desktop, 0, NULL, &folder);
	std::wstring path = folder + std::wstring(L"\\serialized.txt");
	CoTaskMemFree(folder);

	HANDLE file_handle = CreateFileW(path.c_str(), GENERIC_WRITE, FILE_SHARE_DELETE | FILE_SHARE_READ | FILE_SHARE_WRITE , NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
	if (file_handle != INVALID_HANDLE_VALUE) {
		DWORD bytes_written;
		BOOL write_res = WriteFile(file_handle, (TCHAR*)content.c_str(), (DWORD)content.size() * sizeof(TCHAR), &bytes_written, NULL);
		CloseHandle(file_handle);
	}
}

#define Assert(expression) if(!(expression)){*(int*)0=0;}

bool operator==(RECT r1, RECT r2) {
	return r1.left = r2.left && r1.top == r2.top && r1.bottom == r2.bottom && r1.right == r2.right;
}

f64 GetPCFrequency() {
	LARGE_INTEGER li;
	QueryPerformanceFrequency(&li);
	return f64(li.QuadPart) / 1000.0; //milliseconds
}
inline f64 StartCounter() {
	LARGE_INTEGER li;
	QueryPerformanceCounter(&li);
	return f64(li.QuadPart);
}
inline f64 EndCounter(f64 CounterStart, f64 PCFreq = GetPCFrequency())
{
	LARGE_INTEGER li;
	QueryPerformanceCounter(&li);
	return (f64(li.QuadPart) - CounterStart) / PCFreq;
}

int main()
{
	f64 full_time = StartCounter();//timing info

	cl default_client; //Test that default initialization works well for both simple types and structs. NOTE: you aren't requeried to default initialize if you dont want to
	Assert(default_client.b.bottom != 0 && default_client.b.top != 0 && default_client.b.left != 0 && default_client.b.right != 0 && default_client.f!=0.f && default_client.n.a!=0 && default_client.n.br!=0);

	nc nonclient; nonclient.a = 2345; nonclient.br = CreateSolidBrush(RGB(55, 66, 77));
	cl client; client.b = { 98,97,57,56 }; client.f = 4455667788.112233f; client.n.a = 6665; client.n.br = CreateSolidBrush(RGB(255, 223, 180));

	//TODO(for the reader): create a constructor style function called set_to_default(type& var) for each struct from the reflection data you have

	nc copy_nonclient=nonclient;
	cl copy_client=client;

	f64 serialize_time = StartCounter();//timing info

	str serialized;
	_BeginSerialize();
	serialized += _serialize_struct(nonclient);
	serialized += _serialize_struct(client);
	
	serialize_time = EndCounter(serialize_time);//timing info

	save_to_file(serialized);

	nonclient = { 0 };
	client = { 0 };

	const str to_deserialize = load_file();

	f64 deserialize_time = StartCounter();//timing info

	_BeginDeserialize();
	_deserialize_struct(nonclient, to_deserialize);
	_deserialize_struct(client, to_deserialize);

	deserialize_time = EndCounter(deserialize_time);//timing info

	//Test correct deserialization
	Assert(nonclient.a==copy_nonclient.a);
	Assert(ColorFromBrush(nonclient.br)== ColorFromBrush(copy_nonclient.br));
	
	Assert(client.b==copy_client.b);
	Assert(client.f==copy_client.f);
	Assert(client.n.a==copy_client.n.a);
	Assert(ColorFromBrush(client.n.br)== ColorFromBrush(copy_client.n.br));

	DeleteObject(default_client.n.br);//cleanup
	DeleteObject(nonclient.br);
	DeleteObject(copy_nonclient.br);
	DeleteObject(client.n.br);
	DeleteObject(copy_client.n.br);

	printf("ELAPSED FULL:        %f ms\n", EndCounter(full_time));
	printf("ELAPSED SERIALIZE:   %f ms\n",serialize_time);
	printf("ELAPSED DESERIALIZE: %f ms\n",deserialize_time);//NOTE: deserialization is quite a bit slower than serialization

	//TODO(for the very interested reader): implement reflection for enums. And as an extra step save the name instead of the value
	//enum color{red = 24,green = 3,blue = 5}
	//step 1: save and load "24" or "3" or "5"
	//step 2: save and load "red" or "green" or "blue"
}
