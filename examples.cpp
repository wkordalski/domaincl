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

#include "headers.h"
#include "api_abstr.h"
#include "dsl.h"

using namespace domaincl;
namespace api_abstr_example
{
	void run()
	{
		using namespace api_abstr;
		scope gpu("GPU");
		vector<int> v1(30),v2(30),v3(30);
		for(int i=0;i<v1.size();++i)
		{
			v1[i] = i % 10;
			v2[i] = i % 10;
			v3[i] = 0;
		}
		gpu.send_buf("_v1",(void*)&(v1[0]),v1.size()*sizeof(int));
		gpu.send_buf("_v2",(void*)&(v2[0]),v2.size()*sizeof(int));
		gpu.send_buf("_v3",(void*)&(v3[0]),v3.size()*sizeof(int));
		gpu.send_int("size",v1.size());
		#define STRINGIFY(a) #a
		string kod = STRINGIFY(
			__global int* v1 = (__global int*)_v1;
			__global int* v2 = (__global int*)_v2;
			__global int* v3 = (__global int*)_v3;
			v3[i] = v1[i] + v2[i];
		);
		#undef STRINGFY
		gpu.compile_prog("vec_add",make_pair((string)"i",(string)"size"),kod);
		gpu.run_prog("vec_add");
		gpu.fetch_buf("_v3",(void*)&(v3[0]));
		for(int i=0;i<v3.size();++i)
		cout<<v3[i]<<" ";
		cout<<endl;
	}
}

namespace dsl_example
{
	void run()
	{
		using namespace dsl;
		vector<int> a,b,c;
		auto func = [&](val i)
		{
			val aa = a;
			val bb = b;
			val cc = c;
			cc[i] = aa[i] * bb[i];
			
		};
		kernel k = function<void(val)>(func);
		a.resize(10,0);
		b.resize(10,0);
		c.resize(10,0);
		for(int i=0;i<10;++i)
		{
			a[i] = i;
			b[i] = i;
		}
		k.copyin();
		k.run(10);
		k.copyout();
		for(int i=0;i<10;++i)
		cout<<c[i]<<" ";
		cout<<endl;
	}
}

int main()
{
	dsl_example::run();
	return 0;
}
