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
#include "translator.h"

#ifndef DSL_H
#define DSL_H



namespace domaincl
{
	namespace dsl
	{
		using namespace translator;
		
		class val
		{
			id_t nm;
			static env* e;
			friend class kernel;
			val(id_t i):nm(i){}
			static val get_global_id(int a)
			{
				return e->gen_getglobalid(a);
			}
			operator id_t()
			{
				return nm;
			}
			
			public:
			val(vector<int>&v)
			{
				nm = e->eat(v);
			}
			val(vector<float>&v):nm(e->eat(v)){}
			val(int& v):nm(e->eat(v)){}
			val(float& v):nm(e->eat(v)){}
			val(const val&v):nm(v.nm){}
			
			val operator[](val a)
			{
				return e->gen_index(nm,a);
			}
			
			// standardowe operatory:
			val operator=(val a)
			{
				return e->gen_assign(nm,a);
			}
			val operator+(val a)
			{
				return e->gen_binop(nm,"+",a);
			}
			val operator-(val a)
			{
				return e->gen_binop(nm,"-",a);
			}
			val operator*(val a)
			{
				return e->gen_binop(nm,"*",a);
			}
			val operator/(val a)
			{
				return e->gen_binop(nm,"/",a);
			}
			val operator%(val a)
			{
				return e->gen_binop(nm,"%",a);
			}
			val operator&&(val a)
			{
				return e->gen_binop(nm,"&&",a);
			}
			val operator||(val a)
			{
				return e->gen_binop(nm,"||",a);
			}
			val operator&(val a)
			{
				return e->gen_binop(nm,"&",a);
			}
			val operator|(val a)
			{
				return e->gen_binop(nm,"|",a);
			}
			val operator!()
			{
				return e->gen_unop("!",nm);
			}
			val operator==(val a)
			{
				return e->gen_binop(nm,"==",a);
			}
			val operator>=(val a)
			{
				return e->gen_binop(nm,">=",a);
			}
			val operator<=(val a)
			{
				return e->gen_binop(nm,"<=",a);
			}
			val operator>(val a)
			{
				return e->gen_binop(nm,">",a);
			}
			val operator<(val a)
			{
				return e->gen_binop(nm,"<",a);
			}
		};
		env* val::e = NULL;
		
		class kernel
		{
			env* en;
			api_abstr::scope* gpu;
			vector<string> extents;
			int argc;
			
			public:
			kernel(function<void(val)> f) : argc(1)
			{
				en = new env();
				gpu = new api_abstr::scope("gpu");
				val::e = en;
				f(val::get_global_id(0));
				val::e = NULL;
				extents.push_back(id_t::generate().str());
				string c = en->get_code();
				D(c);
				en->declare_deps(*gpu);
				gpu->compile_prog("domaincl_kernel",extents,c);
			}
			// kernel (function<val(val,val)>)
			// kernel (function<val(val,val,val)>) etc.
			void copyin()
			{
				en->copyin(*gpu);
			}
			void copyout()
			{
				en->copyout(*gpu);
			}
			void run(int a)
			{
				if(argc == 1)
				{
					gpu->send_int(extents[0],a);
					gpu->run_prog("domaincl_kernel");
				}
				else throw UNIMPL;
			}
		};
		
	}
}

#endif
