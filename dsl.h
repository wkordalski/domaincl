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
		
		#define DO(c) ([&](int line_end){val::open_block(__LINE__, line_end);{c};val::close_block();})(__LINE__);
		
		class val
		{
			protected:
			
			id_t nm;
			static env* e;
			friend class kernel;
			friend class if_type;
			friend class else_type;
			friend class for_type;
			val(id_t i=id_t()):nm(i){}
			static val get_global_id(int a)
			{
				return e->gen_getglobalid(a);
			}
			
			public:
			
			operator id_t() const
			{
				return nm;
			}
			
			val(vector<int>&v)
			{
				nm = e->eat(v);
			}
			val(vector<float>&v):nm(e->eat(v)){}
			//FIXME FIXME
			//val(int& v):nm(e->eat(v)){}
			//val(float& v):nm(e->eat(v)){}
			val(const val&v):nm(v.nm){}
			
			//FIXME FIXME:
			val(int a)
			{
				int* ptr = new int;
				*ptr = a;
				nm = e->eat(*ptr);
			}
			//FIXME FIXME:
			val(float a)
			{
				float* ptr = new float;
				*ptr = a;
				nm = e->eat(*ptr);
			}
			
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
			
			static void open_block(int l_start, int l_end)
			{
				e->open_block(l_start, l_end);
			}
			static void close_block()
			{
				e->close_block();
			} 
		};
		env* val::e = NULL;
		
		class var : public val
		{
			void copy()
			{
				val::nm = e->gen_copy(id_t(val::nm));
			}
			public:
			var(val a)
			{
				val::nm = e->gen_copy(id_t(a));
			}
			var(int a):val(a){copy();}
			var(float a):val(a){copy();}
			var(vector<int>& a):val(a){copy();}
			var(vector<float>& a):val(a){copy();}
		};
		
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
				//try
				{
					f(val::get_global_id(0));
				}
				/*
				catch (error e)
				{
					string msg = "an error occured during compilation\n";
					msg += en->get_opened_blocks();
					msg += e.message;
					cerr<<msg<<endl;
					exit(0);
				}
				*/
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
		
		class if_type
		{
			public:
			function<function<void(int)>(function<void(int)>)> operator()(val cond){
				return [=](function<void(int)> block)
				{
					return [=](int line)
					{
						val::e->gen_if(cond);
						block(line);
					};
				};
			}
		}If;
		
		class else_type
		{
			public:
			function<void(int)> operator()(function<void(int)> block)
			{
				return [=](int line)
				{
					val::e->gen_else();
					block(line);
				};
			}
		}Else;
		
		class for_type
		{
			public:
			function<function<void(int)>(function<void(int)>)> operator()(val it, val start, val end){
				return [=](function<void(int)> block)
				{
					return [=](int line)
					{
						val::e->gen_for(it, start, end);
						block(line);
					};
				};
			}
		}For;
		
	}
}

#endif
