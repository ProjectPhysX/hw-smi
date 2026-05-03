#pragma once

#define UTILITIES_REGEX
#define UTILITIES_FILE
#define UTILITIES_CONSOLE_COLOR
#define CONSOLE_WIDTH 79
//#define UTILITIES_NO_CPP17

#pragma warning(disable:26451)
#pragma warning(disable:6386)
#pragma warning(disable:6001)
#include <cmath>
#include <vector>
#include <string>
#ifdef UTILITIES_REGEX
#include <regex> // contains <string>, <vector>, <algorithm> and others
#endif // UTILITIES_REGEX
#include <iostream>
#include <chrono>
#include <thread>
#undef min
#undef max
using std::string;
using std::vector;
using std::thread;
typedef unsigned char uchar;
typedef unsigned short ushort;
typedef unsigned int uint;
typedef int64_t slong;
typedef uint64_t ulong;
#define pif 3.1415927f
#define pi 3.141592653589793
#define min_char ((char)-128)
#define max_char ((char)127)
#define max_uchar ((uchar)255)
#define min_short ((short)-32768)
#define max_short ((short)32767)
#define max_ushort ((ushort)65535)
#define min_int ((int)-2147483648)
#define max_int 2147483647
#define max_uint 4294967295u
#define min_slong ((slong)-9223372036854775808ll)
#define max_slong 9223372036854775807ll
#define max_ulong 18446744073709551615ull
#define min_float 1.401298464E-45f
#define max_float 3.402823466E38f
#define epsilon_float 1.192092896E-7f
#define inf_float as_float(0x7F800000)
#define nan_float as_float(0xFFFFFFFF)
#define min_double 4.9406564584124654E-324
#define max_double 1.7976931348623158E308
#define epsilon_double 2.2204460492503131E-16
#define inf_double as_double(0x7FF0000000000000)
#define nan_double as_double(0xFFFFFFFFFFFFFFFF)

class Clock {
private:
	typedef std::chrono::high_resolution_clock clock;
	std::chrono::time_point<clock> t;
public:
	inline Clock() { start(); }
	inline void start() { t = clock::now(); }
	inline double stop() const { return std::chrono::duration_cast<std::chrono::duration<double>>(clock::now()-t).count(); }
};
inline void sleep(const double t) {
	if(t>0.0) std::this_thread::sleep_for(std::chrono::milliseconds((int)(1E3*t+0.5)));
}

template<typename T, uint N> struct CircularBuffer {
	T* data = new T[N];
	uint n = 0u;
	CircularBuffer() {
		for(uint i=0u; i<N; i++) {
			data[i] = (T)0;
		}
	}
	~CircularBuffer() {
		delete[] data;
	}
	void insert(const T& x) {
		data[n] = x;
		n = (n+1u)%N;
	}
	const T& operator[](const uint i) const {
		return data[(n+i)%N];
	}
};

inline float sq(const float x) {
	return x*x;
}
inline float cb(const float x) {
	return x*x*x;
}
inline float pow(const float x, const uint n) {
	float r = 1.0f;
	for(uint i=0u; i<n; i++) {
		r *= x;
	}
	return r;
}
inline float sign(const float x) {
	return x>=0.0f ? 1.0f : -1.0f;
}
inline float clamp(const float x, const float a, const float b) {
	return fmin(fmax(x, a), b);
}
inline float rsqrt(const float x) {
	return 1.0f/sqrt(x);
}
inline float ln(const float x) {
	return log(x); // natural logarithm
}

inline double sq(const double x) {
	return x*x;
}
inline double cb(const double x) {
	return x*x*x;
}
inline double pow(const double x, const uint n) {
	double r = 1.0;
	for(uint i=0u; i<n; i++) {
		r *= x;
	}
	return r;
}
inline double sign(const double x) {
	return x>=0.0 ? 1.0 : -1.0;
}
inline double clamp(const double x, const double a, const double b) {
	return fmin(fmax(x, a), b);
}
inline double rsqrt(const double x) {
	return 1.0/sqrt(x);
}
inline double ln(const double x) {
	return log(x); // natural logarithm
}

inline int sq(const int x) {
	return x*x;
}
inline int cb(const int x) {
	return x*x*x;
}
inline int pow(const int x, const uint n) {
	int r = 1;
	for(uint i=0u; i<n; i++) {
		r *= x;
	}
	return r;
}
inline int sign(const int x) {
	return 1-2*(x>>31&1);
}
inline int min(const int x, const int y) {
	return x<y?x:y;
}
inline int max(const int x, const int y) {
	return x>y?x:y;
}
inline int clamp(const int x, const int a, const int b) {
	return min(max(x, a), b);
}

inline uint sq(const uint x) {
	return x*x;
}
inline uint cb(const uint x) {
	return x*x*x;
}
inline uint pow(const uint x, const uint n) {
	uint r = 1u;
	for(uint i=0u; i<n; i++) {
		r *= x;
	}
	return r;
}
inline uint min(const uint x, const uint y) {
	return x<y?x:y;
}
inline uint max(const uint x, const uint y) {
	return x>y?x:y;
}
inline uint clamp(const uint x, const uint a, const uint b) {
	return min(max(x, a), b);
}
inline uint gcd(uint x, uint y) { // greatest common divisor
	if(x*y==0u) return 0u;
	uint t;
	while(y!=0u) {
		t = y;
		y = x%y;
		x = t;
	}
	return x;
}
inline uint lcm(const uint x, const uint y) { // least common multiple
	return x*y==0u ? 0u : x*y/gcd(x, y);
}
inline uint percentage(const uint current, const uint max) {
	return current<max_uint&&max>0u&&max<max_uint ? clamp((100u*current+max/2u)/max, 0u, 100u) : (uint)max_uchar; // return [0,100] if valid, 255 if invalid
}

inline slong sq(const slong x) {
	return x*x;
}
inline slong cb(const slong x) {
	return x*x*x;
}
inline slong pow(const slong x, const uint n) {
	slong r = 1ll;
	for(uint i=0u; i<n; i++) {
		r *= x;
	}
	return r;
}
inline slong sign(const slong x) {
	return 1ll-2ll*(x>>63&1ll);
}
inline slong min(const slong x, const slong y) {
	return x<y?x:y;
}
inline slong max(const slong x, const slong y) {
	return x>y?x:y;
}
inline slong clamp(const slong x, const slong a, const slong b) {
	return min(max(x, a), b);
}

inline ulong sq(const ulong x) {
	return x*x;
}
inline ulong cb(const ulong x) {
	return x*x*x;
}
inline ulong pow(const ulong x, const uint n) {
	ulong r = 1ull;
	for(uint i=0u; i<n; i++) {
		r *= x;
	}
	return r;
}
inline ulong min(const ulong x, const ulong y) {
	return x<y?x:y;
}
inline ulong max(const ulong x, const ulong y) {
	return x>y?x:y;
}
inline ulong clamp(const ulong x, const ulong a, const ulong b) {
	return min(max(x, a), b);
}
inline ulong gcd(ulong x, ulong y) { // greatest common divisor
	if(x*y==0ull) return 0ull;
	ulong t;
	while(y!=0ull) {
		t = y;
		y = x%y;
		x = t;
	}
	return x;
}
inline ulong lcm(const ulong x, const ulong y) { // least common multiple
	return x*y==0ull ? 0ull : x*y/gcd(x, y);
}

inline int to_int(const float x) {
	return (int)(x+0.5f-(float)(x<0.0f));
}
inline int to_int(const double x) {
	return (int)(x+0.5-(double)(x<0.0));
}
inline uint to_uint(const float x) {
	return (uint)fmax(x+0.5f, 0.5f);
}
inline uint to_uint(const double x) {
	return (uint)fmax(x+0.5, 0.5);
}
inline slong to_slong(const float x) {
	return (slong)(x+0.5f);
}
inline slong to_slong(const double x) {
	return (slong)(x+0.5);
}
inline ulong to_ulong(const float x) {
	return (ulong)fmax(x+0.5f, 0.5f);
}
inline ulong to_ulong(const double x) {
	return (ulong)fmax(x+0.5, 0.5);
}

inline void split_float(float x, uint& integral, uint& decimal, int& exponent) {
	if(x>=10.0f) { // convert to base 10
		if(x>=1E32f) { x *= 1E-32f; exponent += 32; }
		if(x>=1E16f) { x *= 1E-16f; exponent += 16; }
		if(x>= 1E8f) { x *=  1E-8f; exponent +=  8; }
		if(x>= 1E4f) { x *=  1E-4f; exponent +=  4; }
		if(x>= 1E2f) { x *=  1E-2f; exponent +=  2; }
		if(x>= 1E1f) { x *=  1E-1f; exponent +=  1; }
	}
	if(x>0.0f && x<=1.0f) {
		if(x<1E-31f) { x *=  1E32f; exponent -= 32; }
		if(x<1E-15f) { x *=  1E16f; exponent -= 16; }
		if(x< 1E-7f) { x *=   1E8f; exponent -=  8; }
		if(x< 1E-3f) { x *=   1E4f; exponent -=  4; }
		if(x< 1E-1f) { x *=   1E2f; exponent -=  2; }
		if(x<  1E0f) { x *=   1E1f; exponent -=  1; }
	}
	integral = (uint)x;
	float remainder = (x-integral)*1E8f; // 8 decimal digits
	decimal = (uint)remainder;
	if(remainder-(float)decimal>=0.5f) { // correct rounding of last decimal digit
		decimal++;
		if(decimal>=100000000u) { // decimal overflow
			decimal = 0u;
			integral++;
			if(integral>=10u) { // decimal overflow causes integral overflow
				integral = 1u;
				exponent++;
			}
		}
	}
}
inline void split_double(double x, uint& integral, ulong& decimal, int& exponent) {
	if(x>=10.0) { // convert to base 10
		if(x>=1E256) { x *= 1E-256; exponent += 256; }
		if(x>=1E128) { x *= 1E-128; exponent += 128; }
		if(x>= 1E64) { x *=  1E-64; exponent +=  64; }
		if(x>= 1E32) { x *=  1E-32; exponent +=  32; }
		if(x>= 1E16) { x *=  1E-16; exponent +=  16; }
		if(x>=  1E8) { x *=   1E-8; exponent +=   8; }
		if(x>=  1E4) { x *=   1E-4; exponent +=   4; }
		if(x>=  1E2) { x *=   1E-2; exponent +=   2; }
		if(x>=  1E1) { x *=   1E-1; exponent +=   1; }
	}
	if(x>0.0 && x<=1.0) {
		if(x<1E-255) { x *=  1E256; exponent -= 256; }
		if(x<1E-127) { x *=  1E128; exponent -= 128; }
		if(x< 1E-63) { x *=   1E64; exponent -=  64; }
		if(x< 1E-31) { x *=   1E32; exponent -=  32; }
		if(x< 1E-15) { x *=   1E16; exponent -=  16; }
		if(x<  1E-7) { x *=    1E8; exponent -=   8; }
		if(x<  1E-3) { x *=    1E4; exponent -=   4; }
		if(x<  1E-1) { x *=    1E2; exponent -=   2; }
		if(x<   1E0) { x *=    1E1; exponent -=   1; }
	}
	integral = (uint)x;
	double remainder = (x-integral)*1E16; // 16 decimal digits
	decimal = (ulong)remainder;
	if(remainder-(double)decimal>=0.5) { // correct rounding of last decimal digit
		decimal++;
		if(decimal>=10000000000000000ull) { // decimal overflow
			decimal = 0ull;
			integral++;
			if(integral>=10u) { // decimal overflow causes integral overflow
				integral = 1u;
				exponent++;
			}
		}
	}
}
inline string decimal_to_string_float(uint x, int digits) {
	string r = "";
	while((digits--)>0) {
		r = (char)(x%10u+48u)+r;
		x /= 10u;
	}
	return r;
}
inline string decimal_to_string_double(ulong x, int digits) {
	string r = "";
	while((digits--)>0) {
		r = (char)(x%10ull+48ull)+r;
		x /= 10ull;
	}
	return r;
}

inline vector<string> get_main_arguments(int argc, char* argv[]) {
	return argc>1 ? vector<string>(argv+1, argv+argc) : vector<string>();
}

inline string to_string(const string& s){
	return s;
}
inline string to_string(char c) {
	return string(1, c);
}
inline string to_string(uchar c) {
	return string(1, c);
}
inline string to_string(ulong x) {
	string r = "";
	do {
		r = (char)(x%10ull+48ull)+r;
		x /= 10ull;
	} while(x);
	return r;
}
inline string to_string(slong x) {
	return x>=0ll ? to_string((ulong)x) : "-"+to_string((ulong)(-x));
}
inline string to_string(uint x) {
	string r = "";
	do {
		r = (char)(x%10u+48u)+r;
		x /= 10u;
	} while(x);
	return r;
}
inline string to_string(int x) {
	return x>=0 ? to_string((uint)x) : "-"+to_string((uint)(-x));
}
inline string to_string_hex(ulong x) {
	string r = "";
	for(uint i=0u; i<16u; i++) {
		const uint hex_char = (uint)(x&0xFull);
		r = (char)(hex_char+(hex_char<10u ? 48u : 55u))+r;
		x >>= 4u;
	}
	return "0x"+r;
}
inline string to_string_hex(slong x) {
	return to_string_hex(*(ulong*)&x);
}
inline string to_string_hex(uint x) {
	string r = "";
	for(uint i=0u; i<8u; i++) {
		const uint hex_char = x&0xFu;
		r = (char)(hex_char+(hex_char<10u ? 48u : 55u))+r;
		x >>= 4u;
	}
	return "0x"+r;
}
inline string to_string_hex(int x) {
	return to_string_hex(*(uint*)&x);
}
inline string to_string_hex(ushort x) {
	string r = "";
	for(uint i=0u; i<4u; i++) {
		const uint hex_char = x&0xFu;
		r = (char)(hex_char+(hex_char<10u ? 48u : 55u))+r;
		x >>= 4u;
	}
	return "0x"+r;
}
inline string to_string_hex(short x) {
	return to_string_hex(*(ushort*)&x);
}
inline string to_string_hex(uchar x) {
	string r = "";
	for(uint i=0u; i<2u; i++) {
		const uint hex_char = x&0xFu;
		r = (char)(hex_char+(hex_char<10u ? 48u : 55u))+r;
		x >>= 4u;
	}
	return "0x"+r;
}
inline string to_string_hex(char x) {
	return to_string_hex(*(uchar*)&x);
}
inline string to_string(float x) { // convert float to string with full precision (<string> to_string() prints only 6 decimals)
	string s = "";
	if(x<0.0f) { s += "-"; x = -x; }
	if(std::isnan(x)) return s+"NaN";
	if(std::isinf(x)) return s+"Inf";
	uint integral, decimal;
	int exponent = 0;
	split_float(x, integral, decimal, exponent);
	return s+to_string(integral)+"."+decimal_to_string_float(decimal, 8)+(exponent!=0?"E"+to_string(exponent):"");
}
inline string to_string(double x) { // convert double to string with full precision (<string> to_string() prints only 6 decimals)
	string s = "";
	if(x<0.0) { s += "-"; x = -x; }
	if(std::isnan(x)) return s+"NaN";
	if(std::isinf(x)) return s+"Inf";
	uint integral;
	ulong decimal;
	int exponent = 0;
	split_double(x, integral, decimal, exponent);
	return s+to_string(integral)+"."+decimal_to_string_double(decimal, 16)+(exponent!=0?"E"+to_string(exponent):"");
}
inline string to_string(float x, const uint decimals) { // convert float to string with specified number of decimals
	string s = "";
	if(x<0.0f) { s += "-"; x = -x; }
	if(std::isnan(x)) return s+"NaN";
	if(std::isinf(x)||x>(float)max_ulong) return s+"Inf";
	const float power = pow(10.0f, min(decimals, 8u));
	x += 0.5f/power; // rounding
	const ulong integral = (ulong)x;
	const uint decimal = (uint)((x-(float)integral)*power);
	return s+to_string(integral)+(decimals==0u ? "" : "."+decimal_to_string_float(decimal, min((int)decimals, 8)));
}
inline string to_string(double x, const uint decimals) { // convert float to string with specified number of decimals
	string s = "";
	if(x<0.0) { s += "-"; x = -x; }
	if(std::isnan(x)) return s+"NaN";
	if(std::isinf(x)||x>(double)max_ulong) return s+"Inf";
	const double power = pow(10.0, min(decimals, 16u));
	x += 0.5/power; // rounding
	const ulong integral = (ulong)x;
	const ulong decimal = (ulong)((x-(double)integral)*power);
	return s+to_string(integral)+(decimals==0u ? "" : "."+decimal_to_string_double(decimal, min((int)decimals, 16)));
}
template<class T> inline string to_string(const vector<T>& v) { // this vector notation is needed when working with Configuration_File
	string s="[";
	for(uint i=0u; i<(uint)v.size()-1u; i++) {
		s += to_string(v[i]) + ", ";
	}
	s += (uint)v.size()>0u ? to_string(v[(uint)v.size()-1u]) : "";
	return s+"]";
}

inline uint length(const string& s) {
	return (uint)s.length();
}
inline bool contains(const string& s, const string& match) {
	return s.find(match)!=string::npos;
}
inline bool contains_any(const string& s, const vector<string>& matches) {
	for(uint i=0u; i<(uint)matches.size(); i++) if(contains(s, matches[i])) return true;
	return false;
}
inline string to_lower(const string& s) {
	string r = "";
	for(uint i=0u; i<(uint)s.length(); i++) {
		const uchar c = s.at(i);
		r += c>64u&&c<91u ? c+32u : c;
	}
	return r;
}
inline string to_upper(const string& s) {
	string r = "";
	for(uint i=0u; i<(uint)s.length(); i++) {
		const uchar c = s.at(i);
		r += c>96u&&c<123u ? c-32u : c;
	}
	return r;
}
inline bool equals(const string& a, const string& b) {
	return to_lower(a)==to_lower(b);
}
inline string replace(const string& s, const string& from, const string& to) {
	string r = s;
	int p = 0;
	while((p=(int)r.find(from, p))!=string::npos) {
		r.replace(p, from.length(), to);
		p += (int)to.length();
	}
	return r;
}
inline string substring(const string& s, const uint start, uint length=max_uint) {
	return s.substr(start, min(length, (uint)s.length()-start));
}
inline string trim(const string& s) { // removes whitespace characters from beginnig and end of string s
	const int l = (int)s.length();
	int a=0, b=l-1;
	char c;
	while(a<l && ((c=s[a])==' '||c=='\t'||c=='\n'||c=='\v'||c=='\f'||c=='\r'||c=='\0')) a++;
	while(b>a && ((c=s[b])==' '||c=='\t'||c=='\n'||c=='\v'||c=='\f'||c=='\r'||c=='\0')) b--;
	return s.substr(a, 1+b-a);
}
inline bool begins_with(const string& s, const string& match) {
	if(match.size()>s.size()) return false;
	else return equal(match.begin(), match.end(), s.begin());
}
inline bool ends_with(const string& s, const string& match) {
	if(match.size()>s.size()) return false;
	else return equal(match.rbegin(), match.rend(), s.rbegin());
}
template<class T> inline bool contains(const vector<T>& v, const T& match) {
	return find(v.begin(), v.end(), match)!=v.end();
}

inline string alignl(const uint n, const string& x="") { // converts x to string with spaces behind such that length is n if x is not longer than n
	string s = x;
	for(uint i=0u; i<n; i++) s += " ";
	return s.substr(0, max(n, (uint)x.length()));
}
inline string alignr(const uint n, const string& x="") { // converts x to string with spaces in front such that length is n if x is not longer than n
	string s = "";
	for(uint i=0u; i<n; i++) s += " ";
	s += x;
	return s.substr((uint)min((int)s.length()-(int)n, (int)n), s.length());
}
template<typename T> inline string alignl(const uint n, const T x) { // converts x to string with spaces behind such that length is n if x does not have more digits than n
	return alignl(n, to_string(x));
}
template<typename T> inline string alignr(const uint n, const T x) { // converts x to string with spaces in front such that length is n if x does not have more digits than n
	return alignr(n, to_string(x));
}

inline string print_time(const double t) { // input: time in seconds, output: formatted string ___y___d__h__m__s
	uint s = (uint)(t+0.5);
	const uint y = s/31556925; s %= 31556925;
	const uint d = s/   86400; s %=    86400;
	const uint h = s/    3600; s %=     3600;
	const uint m = s/      60; s %=       60;
	const bool dy=t>31556924.5, dd=t>86399.5, dh=t>3599.5, dm=t>59.5; // display years, days, hours, minutes?
	return (dy?                                 to_string(y)+"y":"")+
	       (dd?(dy&&d<10?"00":dy&&d<100?"0":"")+to_string(d)+"d":"")+
	       (dh?(dd&&h<10?"0"               :"")+to_string(h)+"h":"")+
	       (dm?(dh&&m<10?"0"               :"")+to_string(m)+"m":"")+
	           (dm&&s<10?"0"               :"")+to_string(s)+"s"    ;
}
inline string repeat(const string& s, const uint n) {
	string r = "";
	for(uint i=0u; i<n; i++) r += s;
	return r;
}
inline string print_percentage(const float x) {
	return alignr(3u, to_uint(100.0f*x))+"%";
}
inline string print_progress(const float x, const int n=10) {
	const int p = to_int((float)(n-7)*x);
	string s = "[";
	for(int i=0; i<p; i++) s += "=";
	for(int i=p; i<n-7; i++) s += " ";
	return s+"] "+print_percentage(x);
}
inline string d2(const uint n, const string f=" ") {
	string s = f+to_string(n);
	return s.substr(s.length()-2, s.length());
}
inline string d3(const uint n, const string f="  ") {
	string s = f+to_string(n);
	return s.substr(s.length()-3, s.length());
}
inline string d4(const uint n, const string f="   ") {
	string s = f+to_string(n);
	return s.substr(s.length()-4, s.length());
}
inline string d5(const uint n, const string f="    ") {
	string s = f+to_string(n);
	return s.substr(s.length()-5, s.length());
}
inline string d6(const uint n, const string f="     ") {
	string s = f+to_string(n);
	return s.substr(s.length()-6, s.length());
}
inline string d7(const uint n, const string f="      ") {
	string s = f+to_string(n);
	return s.substr(s.length()-7, s.length());
}
inline string d8(const uint n, const string f="       ") {
	string s = f+to_string(n);
	return s.substr(s.length()-8, s.length());
}
inline string d9(const uint n, const string f="        ") {
	string s = f+to_string(n);
	return s.substr(s.length()-9, s.length());
}
inline string d10(const uint n, const string f="         ") {
	string s = f+to_string(n);
	return s.substr(s.length()-10, s.length());
}

inline void print(const string& s="") {
	std::cout << s;
}
inline void println(const string& s="") {
	std::cout << s+"\n";
}
inline void reprint(const string& s="") {
	std::cout << "\r"+s;
}
inline void wait() {
	std::cin.get();
}
template<typename T> inline void println(const T& x) {
	println(to_string(x));
}

inline int color(const int red, const int green, const int blue) {
	return (red&255)<<16|(green&255)<<8|(blue&255);
}
inline int color(const int red, const int green, const int blue, const int alpha) {
	return (alpha&255)<<24|(red&255)<<16|(green&255)<<8|(blue&255);
}
inline int color(const float red, const float green, const float blue) {
	return clamp((int)(255.0f*red+0.5f), 0, 255)<<16|clamp((int)(255.0f*green+0.5f), 0, 255)<<8|clamp((int)(255.0f*blue+0.5f), 0, 255);
}
inline int color(const float red, const float green, const float blue, const float alpha) {
	return clamp((int)(255.0f*alpha+0.5f), 0, 255)<<24|clamp((int)(255.0f*red+0.5f), 0, 255)<<16|clamp((int)(255.0f*green+0.5f), 0, 255)<<8|clamp((int)(255.0f*blue+0.5f), 0, 255);
}
inline int red(const int color) {
	return (color>>16)&255;
}
inline int green(const int color) {
	return (color>>8)&255;
}
inline int blue(const int color) {
	return color&255;
}
inline int alpha(const int color) {
	return (color>>24)&255;
}

#define color_black      0
#define color_dark_blue  1
#define color_dark_green 2
#define color_light_blue 3
#define color_dark_red   4
#define color_magenta    5
#define color_orange     6
#define color_light_gray 7
#define color_gray       8
#define color_blue       9
#define color_green     10
#define color_cyan      11
#define color_red       12
#define color_pink      13
#define color_yellow    14
#define color_white     15

#ifdef UTILITIES_CONSOLE_INPUT
#define UTILITIES_CONSOLE_COLOR
#endif // UTILITIES_CONSOLE_INPUT
#ifdef UTILITIES_CONSOLE_COLOR
#if defined(_WIN32)
#ifndef UTILITIES_REGEX
#include <algorithm> // for transform() in get_exe_path()
#endif // UTILITIES_REGEX
#define WIN32_LEAN_AND_MEAN
#define VC_EXTRALEAN
#include <Windows.h> // for displaying colors and getting console size
#undef min
#undef max
#elif defined(__linux__)||defined(__APPLE__)
#include <sys/ioctl.h> // for getting console size
#include <unistd.h> // for getting path of executable
#if defined(__APPLE__)
#include <mach-o/dyld.h> // _NSGetExecutablePath
#endif // Apple
#else // Linux or Apple
#undef UTILITIES_CONSOLE_COLOR
#endif // Windows/Linux
#endif // UTILITIES_CONSOLE_COLOR
#ifdef UTILITIES_CONSOLE_COLOR
inline string get_exe_path() { // returns path where executable is located, ends with a "/"
	string path = "";
#if defined(_WIN32)
	wchar_t wc[260] = {0};
	GetModuleFileNameW(NULL, wc, 260);
	std::wstring ws(wc);
	transform(ws.begin(), ws.end(), back_inserter(path), [](wchar_t c) { return (char)c; });
	path = replace(path, "\\", "/");
#elif defined(__APPLE__)
	uint length = 0u;
	_NSGetExecutablePath(nullptr, &length);
	path.resize(length+1u, 0);
	_NSGetExecutablePath(path.data(), &length);
#else // Linux
	char c[260];
	int length = (int)readlink("/proc/self/exe", c, 260);
	path = string(c, length>0 ? length : 0);
#endif // Windows/Linux
	return path.substr(0, path.rfind('/')+1);
}
inline void get_console_size(uint& width, uint& height) {
#if defined(_WIN32)
	static const HANDLE handle = GetStdHandle(STD_OUTPUT_HANDLE);
	CONSOLE_SCREEN_BUFFER_INFO csbi;
	GetConsoleScreenBufferInfo(handle, &csbi);
	width = (uint)(csbi.srWindow.Right-csbi.srWindow.Left+1); // (uint)(csbi.dwSize.X); gives size of screen buffer
	height = (uint)(csbi.srWindow.Bottom-csbi.srWindow.Top+1); // (uint)(csbi.dwSize.Y); gives size of screen buffer
#else // Linux
	struct winsize w;
	ioctl(fileno(stdout), TIOCGWINSZ, &w);
	width = (uint)(w.ws_col);
	height = (uint)(w.ws_row);
#endif // Windows/Linux
}
inline void get_console_font_size(uint& width, uint& height) {
#if defined(_WIN32)
	static const HANDLE handle = GetStdHandle(STD_OUTPUT_HANDLE);
	CONSOLE_FONT_INFO cfi;
	GetCurrentConsoleFont(handle, false, &cfi);
	width = (uint)(cfi.dwFontSize.X);
	height = (uint)(cfi.dwFontSize.Y);
#else // Linux
	//struct winsize w;
	//ioctl(fileno(stdout), TIOCGWINSZ, &w);
	width = 8u;//(uint)(w.ws_xpixel/w.ws_col);
	height = 16u;//(uint)(w.ws_ypixel/w.ws_row);
#endif // Windows/Linux
}
inline void set_console_cursor(const uint x, const uint y) {
#if defined(_WIN32)
	static const HANDLE handle = GetStdHandle(STD_OUTPUT_HANDLE);
	SetConsoleCursorPosition(handle, {(short)x, (short)y});
#else // Linux
	std::cout << "\033["+to_string(y+1u)+";"+to_string(x+1u)+"f";
#endif // Windows/Linux
}
inline void show_console_cursor(const bool show) {
#if defined(_WIN32)
	static const HANDLE handle = GetStdHandle(STD_OUTPUT_HANDLE);
	CONSOLE_CURSOR_INFO cci;
	GetConsoleCursorInfo(handle, &cci);
	cci.bVisible = show; // show/hide cursor
	SetConsoleCursorInfo(handle, &cci);
#else // Linux
	std::cout << (show ? "\033[?25h" : "\033[?25l"); // show/hide cursor
#endif // Windows/Linux
}
inline void clear_console() {
#if defined(_WIN32)
	static const HANDLE handle = GetStdHandle(STD_OUTPUT_HANDLE);
	CONSOLE_SCREEN_BUFFER_INFO csbi;
	const COORD topLeft = { 0, 0 };
	std::cout.flush(); // cout uses a buffer to batch writes to the underlying console, we don't want stale buffered text to randomly be written out
	GetConsoleScreenBufferInfo(handle, &csbi); // figure out the current width and height of the console window
	const DWORD length = csbi.dwSize.X*csbi.dwSize.Y;
	DWORD written;
	FillConsoleOutputCharacter(handle, TEXT(' '), length, topLeft, &written); // flood-fill the console with spaces to clear it
	FillConsoleOutputAttribute(handle, csbi.wAttributes, length, topLeft, &written); // reset attributes of every character to default, this clears all background colour formatting
	SetConsoleCursorPosition(handle, topLeft); // move the cursor back to the top left for the next sequence of writes
#else // Linux
	std::cout << "\033[2J";
#endif // Windows/Linux
}
inline int get_console_color(const int color) {
	const int r=(color>>16)&255, g=(color>>8)&255, b=color&255;
	const int matches[16] = {
		sq(r-  0)+sq(g-  0)+sq(b-  0), // color_black      0   0   0   0
		sq(r-  0)+sq(g- 55)+sq(b-218), // color_dark_blue  1   0  55 218
		sq(r- 19)+sq(g-161)+sq(b- 14), // color_dark_green 2  19 161  14
		sq(r- 58)+sq(g-150)+sq(b-221), // color_light_blue 3  58 150 221
		sq(r-197)+sq(g- 15)+sq(b- 31), // color_dark_red   4 197  15  31
		sq(r-136)+sq(g- 23)+sq(b-152), // color_magenta    5 136  23 152
		sq(r-193)+sq(g-156)+sq(b-  0), // color_orange     6 193 156   0
		sq(r-204)+sq(g-204)+sq(b-204), // color_light_gray 7 204 204 204
		sq(r-118)+sq(g-118)+sq(b-118), // color_gray       8 118 118 118
		sq(r- 59)+sq(g-120)+sq(b-255), // color_blue       9  59 120 255
		sq(r- 22)+sq(g-198)+sq(b- 12), // color_green     10  22 198  12
		sq(r- 97)+sq(g-214)+sq(b-214), // color_cyan      11  97 214 214
		sq(r-231)+sq(g- 72)+sq(b- 86), // color_red       12 231  72  86
		sq(r-180)+sq(g-  0)+sq(b-158), // color_pink      13 180   0 158
		sq(r-249)+sq(g-241)+sq(b-165), // color_yellow    14 249 241 165
		sq(r-242)+sq(g-242)+sq(b-242)  // color_white     15 242 242 242
	};
	int m=195075, k=0;
	for(int i=0; i<16; i++) if(matches[i]<m) m = matches[k=i];
	return k;
}
inline string get_textcolor_code(const int textcolor) { // Linux only
	switch(textcolor) {
		case  0: return "30"; // color_black      0
		case  1: return "34"; // color_dark_blue  1
		case  2: return "32"; // color_dark_green 2
		case  3: return "36"; // color_light_blue 3
		case  4: return "31"; // color_dark_red   4
		case  5: return "35"; // color_magenta    5
		case  6: return "33"; // color_orange     6
		case  7: return "37"; // color_light_gray 7
		case  8: return "90"; // color_gray       8
		case  9: return "94"; // color_blue       9
		case 10: return "92"; // color_green     10
		case 11: return "96"; // color_cyan      11
		case 12: return "91"; // color_red       12
		case 13: return "95"; // color_pink      13
		case 14: return "93"; // color_yellow    14
		case 15: return "97"; // color_white     15
		default: return "37";
	}
}
inline string get_backgroundcolor_code(const int backgroundcolor) { // Linux only
	switch(backgroundcolor) {
		case  0: return  "40"; // color_black      0
		case  1: return  "44"; // color_dark_blue  1
		case  2: return  "42"; // color_dark_green 2
		case  3: return  "46"; // color_light_blue 3
		case  4: return  "41"; // color_dark_red   4
		case  5: return  "45"; // color_magenta    5
		case  6: return  "43"; // color_orange     6
		case  7: return  "47"; // color_light_gray 7
		case  8: return "100"; // color_gray       8
		case  9: return "104"; // color_blue       9
		case 10: return "102"; // color_green     10
		case 11: return "106"; // color_cyan      11
		case 12: return "101"; // color_red       12
		case 13: return "105"; // color_pink      13
		case 14: return "103"; // color_yellow    14
		case 15: return "107"; // color_white     15
		default: return  "40";
	}
}
inline string get_print_color(const int textcolor) { // Linux only
	return "\033["+get_textcolor_code(textcolor)+"m";
}
inline string get_print_color(const int textcolor, const int backgroundcolor) { // Linux only
	return "\033["+get_textcolor_code(textcolor)+";"+get_backgroundcolor_code(backgroundcolor)+"m";
}
inline void print_color(const int textcolor) {
#if defined(_WIN32)
	static const HANDLE handle = GetStdHandle(STD_OUTPUT_HANDLE);
	SetConsoleTextAttribute(handle, textcolor);
#else // Linux
	std::cout << get_print_color(textcolor);
#endif // Windows/Linux
}
inline void print_color(const int textcolor, const int backgroundcolor) {
#if defined(_WIN32)
	static const HANDLE handle = GetStdHandle(STD_OUTPUT_HANDLE);
	SetConsoleTextAttribute(handle, backgroundcolor<<4|textcolor);
#else // Linux
	std::cout << get_print_color(textcolor, backgroundcolor);
#endif // Windows/Linux
}
inline void print_color_reset() {
#if defined(_WIN32)
	static const HANDLE handle = GetStdHandle(STD_OUTPUT_HANDLE);
	SetConsoleTextAttribute(handle, 7); // reset color
#else // Linux
	std::cout << "\033[0m"; // reset color
#endif // Windows/Linux
}
inline void print(const string& s, const int textcolor) {
	print_color(textcolor);
	std::cout << s;
	print_color_reset();
}
inline void print(const string& s, const int textcolor, const int backgroundcolor) {
	print_color(textcolor, backgroundcolor);
	std::cout << s;
	print_color_reset();
}
inline void println(const string& s, const int textcolor) {
	print_color(textcolor);
	std::cout << s+"\n";
	print_color_reset();
}
inline void println(const string& s, const int textcolor, const int backgroundcolor) {
	print_color(textcolor, backgroundcolor);
	std::cout << s+"\n";
	print_color_reset();
}
inline void print_no_reset(const string& s, const int textcolor) { // print with color, but don't reset color afterwards (faster)
	print_color(textcolor);
	std::cout << s;
}
inline void print_no_reset(const string& s, const int textcolor, const int backgroundcolor) { // print with color, but don't reset color afterwards (faster)
	print_color(textcolor, backgroundcolor);
	std::cout << s;
}
inline void print_color_test() {
#ifdef _WIN32
	const string s = string("")+(char)223; // trick to double vertical resolution: use graphic character
#else // Linux
	const string s = "\u2580"; // trick to double vertical resolution: use graphic character
#endif // Windows/Linux
	print(s, color_magenta   , color_black     );
	print(s, color_blue      , color_gray      );
	print(s, color_light_blue, color_light_gray);
	print(s, color_cyan      , color_white     );
	print(s, color_green     , color_pink      );
	print(s, color_yellow    , color_dark_blue );
	print(s, color_orange    , color_dark_green);
	print(s, color_red       , color_dark_red  );
	println();
}
#else // UTILITIES_CONSOLE_COLOR
inline void print(const string& s, const int textcolor) {
	std::cout << s;
}
inline void print(const string& s, const int textcolor, const int backgroundcolor) {
	std::cout << s;
}
#endif // UTILITIES_CONSOLE_COLOR

#ifdef UTILITIES_REGEX
inline vector<string> split_regex(const string& s, const string& separator="\\s+") {
	vector<string> r;
	const std::regex rgx(separator);
	std::sregex_token_iterator token(s.begin(), s.end(), rgx, -1), end;
	while(token!=end) {
		r.push_back(*token);
		token++;
	}
	return r;
}
inline bool equals_regex(const string& s, const string& match) { // returns true if string exactly matches regex
	return regex_match(s.begin(), s.end(), std::regex(match));
}
inline uint matches_regex(const string& s, const string& match) { // counts number of matches
	std::regex words_regex(match);
	auto words_begin = std::sregex_iterator(s.begin(), s.end(), words_regex);
	auto words_end = std::sregex_iterator();
	return (uint)std::distance(words_begin, words_end);
}
inline bool contains_regex(const string& s, const string& match) {
	return matches_regex(s, match)>=1;
}
inline string replace_regex(const string& s, const string& from, const string& to) {
	return regex_replace(s, std::regex(from), to);
}
inline bool is_number(const string& s) {
	return equals_regex(s, "\\d+(u|l|ul|ll|ull)?")||equals_regex(s, "0x(\\d|[a-fA-F])+(u|l|ul|ll|ull)?")||equals_regex(s, "0b[01]+(u|l|ul|ll|ull)?")||equals_regex(s, "(((\\d+\\.?\\d*|\\.\\d+)([eE][+-]?\\d+[fF]?)?)|(\\d+\\.\\d*|\\.\\d+)[fF]?)");
}
inline void parse_sanity_check_error(const string& s, const string& regex, const string& type) {
	if(!equals_regex(s, regex)) println("\""+s+"\" cannot be parsed to "+type+".");
}
inline int to_int(const string& s) {
	const string t = trim(s);
	parse_sanity_check_error(t, "[+-]?\\d+", "int");
	return atoi(t.c_str());
}
inline uint to_uint(const string& s) {
	const string t = trim(s);
	parse_sanity_check_error(t, "\\+?\\d+", "uint");
	return (uint)atoi(t.c_str());
}
inline slong to_slong(const string& s) {
	const string t = trim(s);
	parse_sanity_check_error(t, "[+-]?\\d+", "slong");
	return (slong)atoll(t.c_str());
}
inline ulong to_ulong(const string& s) {
	const string t = trim(s);
	parse_sanity_check_error(t, "\\+?\\d+", "ulong");
	return (ulong)atoll(t.c_str());
}
inline float to_float(const string& s) {
	const string t = trim(s);
	parse_sanity_check_error(t, "[+-]?(((\\d+\\.?\\d*|\\.\\d+)([eE][+-]?\\d+[fF]?)?)|(\\d+\\.\\d*|\\.\\d+)[fF]?)", "float");
	return (float)atof(t.c_str());
}
inline double to_double(const string& s) {
	const string t = trim(s);
	parse_sanity_check_error(t, "[+-]?(((\\d+\\.?\\d*|\\.\\d+)([eE][+-]?\\d+[fF]?)?)|(\\d+\\.\\d*|\\.\\d+)[fF]?)", "double");
	return atof(t.c_str());
}

inline bool parse_sanity_check(const string& s, const string& regex) {
	return equals_regex(s, regex);
}
inline int to_int(const string& s, const int default_value) {
	const string t = trim(s);
	return parse_sanity_check(t, "[+-]?\\d+") ? atoi(t.c_str()) : default_value;
}
inline uint to_uint(const string& s, const uint default_value) {
	const string t = trim(s);
	return parse_sanity_check(t, "\\+?\\d+") ? (uint)atoi(t.c_str()) : default_value;
}
inline slong to_slong(const string& s, const slong default_value) {
	const string t = trim(s);
	return parse_sanity_check(t, "[+-]?\\d+") ? (slong)atoll(t.c_str()) : default_value;
}
inline ulong to_ulong(const string& s, const ulong default_value) {
	const string t = trim(s);
	return parse_sanity_check(t, "\\+?\\d+") ? (ulong)atoll(t.c_str()) : default_value;
}
inline float to_float(const string& s, const float default_value) {
	const string t = trim(s);
	return parse_sanity_check(t, "[+-]?(((\\d+\\.?\\d*|\\.\\d+)([eE][+-]?\\d+[fF]?)?)|(\\d+\\.\\d*|\\.\\d+)[fF]?)") ? (float)atof(t.c_str()) : default_value;
}
inline double to_double(const string& s, const double default_value) {
	const string t = trim(s);
	return parse_sanity_check(t, "[+-]?(((\\d+\\.?\\d*|\\.\\d+)([eE][+-]?\\d+[fF]?)?)|(\\d+\\.\\d*|\\.\\d+)[fF]?)") ? atof(t.c_str()) : default_value;
}
#endif // UTILITIES_REGEX

#ifdef UTILITIES_FILE
#include <fstream> // read/write files
#ifndef UTILITIES_NO_CPP17
#include <filesystem> // automatically create directory before writing file, requires C++17
inline vector<string> find_files(const string& path, const string& extension=".*") {
	vector<string> files;
	if(std::filesystem::is_directory(path)&&std::filesystem::exists(path)) {
		for(const auto& entry : std::filesystem::directory_iterator(path)) {
			if(extension==".*"||entry.path().extension().string()==extension) files.push_back(entry.path().string());
		}
	}
	return files;
}
#endif // UTILITIES_NO_CPP17
inline void create_folder(const string& path) { // create folder if it not already exists
	const int slash_position = (int)path.rfind('/'); // find last slash dividing the path from the filename
	if(slash_position==(int)string::npos) return; // no slash found
	const string f = path.substr(0, slash_position); // cut off file name if there is any
#ifndef UTILITIES_NO_CPP17
	if(!std::filesystem::is_directory(f)||!std::filesystem::exists(f)) std::filesystem::create_directories(f); // create folder if it not already exists
#endif // UTILITIES_NO_CPP17
}
inline string create_file_extension(const string& filename, const string& extension) {
	return filename.substr(0, filename.rfind('.'))+(extension.at(0)!='.'?".":"")+extension; // remove existing file extension if existing and replace it with new one
}
inline string read_file(const string& filename) {
	std::ifstream file(filename, std::ios::in);
	string r = "";
	if(!file.fail()) {
		try {
			r = string((std::istreambuf_iterator<char>(file)), std::istreambuf_iterator<char>());
		} catch(...) {}
	}
	file.close();
	return r;
}
inline void write_file(const string& filename, const string& content="") {
	create_folder(filename);
	std::ofstream file(filename, std::ios::out);
	file.write(content.c_str(), content.length());
	file.close();
}
inline void write_line(const string& filename, const string& content="") {
	const string s = content+(ends_with(content, "\n")?"":"\n"); // add new line if needed
	std::ofstream file(filename, std::ios::ios_base::app);
	file.write(s.c_str(), s.length());
	file.close();
}
template<typename T> inline void write_file(const string& filename, const string& header, const uint n, const T* y) {
	string s = header;
	if(length(s)>0u && !ends_with(s, "\n")) s += "\n";
	for(uint i=0u; i<n; i++) s += to_string(i)+"\t"+to_string(y[i])+"\n";
	write_file(filename, s);
}
template<typename T, typename U> inline void write_file(const string& filename, const string& header, const uint n, const T* x, const U* y) {
	string s = header;
	if(length(s)>0u && !ends_with(s, "\n")) s += "\n";
	for(uint i=0u; i<n; i++) s += to_string(x[i])+"\t"+to_string(y[i])+"\n";
	write_file(filename, s);
}
#endif // UTILITIES_FILE