/**
	Copyright (C) 2014 Franciszek Piszcz
	
	Distributed under the GNU Lesser General Public License, version 3
	
	THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
	IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
	FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
	AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
	LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
	OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
	SOFTWARE.
*/


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
