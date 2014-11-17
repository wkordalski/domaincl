#ifndef HEADERS_CPP
#define HEADERS_CPP

#include<iostream>
#include<cstdio>
#include<vector>
#include<map>
#include<algorithm>
#include<CL/cl.h>

using namespace std;
//#define D(a) (cout<<#a<<" = "<<(a)<<endl,(a))
#define D(a) (a)

#define ERROR(a) (cout<<(a)<<endl,(a))
#define WTF ERROR("What a terrible failure!");
#define UNIMPL ERROR("Unimplemented!");

class error
{
	public:
	error(string s){}
};

#endif
