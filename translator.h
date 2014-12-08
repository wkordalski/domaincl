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

#ifndef TRANSLATOR_H
#define TRANSLATOR_H

namespace domaincl
{
	namespace translator
	{
		class id_t
		{
			string name;
			int id_num;
			static int num;
			id_t(int a,string prefix):id_num(a)
			{
				name = prefix+std::to_string(a);
			}
			public:
			id_t(){}
			id_t(const id_t& a)
			{
				//D(name);
				//D(id_num);
				name = a.name;
				id_num = a.id_num;
			}
			string str() const
			{
				return name;
			}
			static id_t generate(string p = "id")
			{
				num++;
				return id_t(num,p);
			}
			bool operator<(const id_t& i) const
			{
				return id_num < i.id_num;
			}
		};
		int id_t::num = 0;
		
		/*
			1. Reprezentacja wartości w opencl c:
				- refy jako __global void*
				- valtypes jako ich typy - tylko float, int i ew. double
				- varrefs jako j.w.
				- vec sizes jako int
		*/
		
		class instr_stack
		{
			vector<string> v;
			vector<bool> scolon; //czy dodać średnik po instr
			vector<pair<int,int> > blocks;
			friend class env;
			public:
			void add(string i)
			{
				v.push_back(i);
				scolon.push_back(true);
			}
			void add_if(string cond)
			{
				v.push_back("if("+cond+")");
				scolon.push_back(false);
			}
			void open(int l_start, int l_end)
			{
				v.push_back("{");
				scolon.push_back(false);
				blocks.push_back(make_pair(l_start, l_end));
			}
			void close()
			{
				v.push_back("}");
				scolon.push_back(false);
				blocks.pop_back();
			}
			void add_else()
			{
				v.push_back("else");
				scolon.push_back(false);
			}
			
			void add_for(string it, string p, string q)
			{
				v.push_back("for("+it+" = "+p+"; "+it+" <= "+q+"; ++"+it+")");
				scolon.push_back(false);
			}
			
			void add_while(); //TODO
			// pętlę while wbrew pozorom da się zrobić, tylko w tym vectorze instr.
			// trzeba zapisywać coś takiego, jak miejsce ostatniego wywołania konstruktora
			// kopiującego typ val/var, oraz częściowo drzewko AST. (zakładamy, że jakieś działanie na typie val nie wywołuje
			// konstruktora kopiującego, tak samo konstr. przyjmujący zmienną globalną nie
			// jest konstruktorem kopiującym)
			
			string get_code()
			{
				string res = "";
				for(int i=0;i<v.size();++i)
				{
					res += v[i];
					if(scolon[i]) res += ";";
					res += "\n";
				}
				return res;
			}
		};
		// w tę jedną klasę jest wrzucony cały syf generowania kodu:
		class type
		{
			// FIXME <-> do dopracowania szczegółów
			// TODO <-> do zrobienia
			string name;		// type constructor name: vector, float, int, pair
			bool is_ref;	// either is_ref or is_varref
			bool is_varref; // only for primitive types
			vector<type> params; // type constructor params, zawsze już są z refami - WAŻNE przy tworzeniu typów
			id_t vec_size; // only for vectors, describes their size, in openclc it is of type int
			// vectory i pary tylko pod postacią refów
			// args[0] -> wynik, ew. w przypisaniu też jest lewym argumentem
			// args[1] i args[2] -> lewy i prawy argument
			// bo to generuje kompletny 3-addr code, razem z przypisaniem
			private:
			friend class globvar;
			friend ostream& operator<<(ostream&,const type&);
			static type mk_valtype(string s)	//użyte w klasie type
			{
				if(s!="int" && s!="float") throw ERROR("valtypes are only ints and floats");
				type t;
				t.name = s;
				t.is_ref = false;
				t.is_varref = false;
				return t;
			}
			static type mk_varref(string s)	//użyte w klasie type
			{
				if(s!="int" && s!="float") throw ERROR("varrefs are anly ints and floats");
				type t;
				t.name = s;
				t.is_ref = false;
				t.is_varref = true;
				return t;
			}
			static type mk_reftype(string s)  //użyte w klasie globvar
			{
				if(s!="int" && s!="float") throw UNIMPL;
				type t;
				t.name = s;
				t.is_ref = true;
				t.is_varref = false;
				return t;
			}
			static type mk_vectype(const type& el, id_t rozm)  //użyte w klasie globvar
			{
				if(!el.is_ref) throw ERROR("vectype elem has to be a reftype!");
				type t;
				t.name = "vector";
				t.is_ref = true;
				t.is_varref = false;
				t.params.push_back(el);
				t.vec_size = rozm;
				return t;
			}
			// metody zaczynające się od gen zwracają kod openclc
			string gen_deref(string a) const	// of openclc type int or float
			{
				if(name != "int" && name != "float")
				throw ERROR("You can only deref ints and floats!");
				if(!is_ref) return "("+a+")";
				if(name == "int")
				return "(*(__global int*)"+a+")";
				if(name == "float")
				return "(*(__global float*)"+a+")";
			}
			string gen_sizeof()	// of openclc type int
			{
				if(name == "float" || name == "int") return "4";
				if(name == "vector") return "("+vec_size.str()+"*"+params[0].gen_sizeof()+")";
				if(name == "pair") return "("+params[0].gen_sizeof()+"+"+params[1].gen_sizeof()+")";
				throw WTF;
			}
			string gen_deref_type()
			{
				if(name == "float" || name == "int") return name;
				else throw ERROR("Coś poszło nie tak! (nieref do obiektu innego niż int i float)");
			}
			// check_sth -> jak jest błąd, to rzuca wyjątek
			// te metody ogarniają błędy kompilacji
			void check_wtf()
			{
				// FIXME: wywołania tego dodać wszędzie indziej (taka tam paranoia)
				if(is_ref && is_varref) throw WTF;
				if(name == "vector" || name == "int" || name == "float" || name == "pair");
				else throw WTF;
			}
			
			public:
			
			type check_unop(string op)
			{
				if(name != "int" && name != "float")
				throw ERROR("Unary operators can take only floats and ints");
				if(name == "int")
				{
					if(op == "+" || op == "-" || op == "!") return mk_valtype("int");
					else throw ERROR("Other un operators and builtin funcs are unimplemented"); //FIXME
				}
				if(name == "float")
				{
					if(op == "+" || op == "-") return mk_valtype("float");
					else throw ERROR("Other un operators and builtin funcs are unimplemented"); //FIXME
				}
			}
			string gen_unop(string op, vector<string> args) //DONE
			{
				return gen_deref_type()+" "+args[0]+" = ("+op+gen_deref(args[1])+")";
			}
			type check_first()
			{
				if(name != "pair") throw ERROR("Trying to get first element of a non-pair object");
				return params[0];
			}
			string gen_first(vector<string> args) //DONE
			{
				return "__global void* "+args[0]+" = "+args[1];
			}
			type check_second()
			{
				if(name != "pair") throw ERROR("Trying to get second element of a non-pair object");
				return params[1];
			}
			string gen_second(vector<string> args) //DONE
			{
				return "__global void* "+args[0]+" = "+args[1]+"+"+params[0].gen_sizeof();
			}
			type check_binop(string op,const type& t)
			{
				if(t.name == "int" && name == "int")
				{
					if(op == "+" || op == "-" || op == "/" || op == "%" || op == "*" || op == "==" || op == "<=") return mk_valtype("int");
					else throw ERROR("Unknown operator"); //FIXME
				}
				if(t.name == "float" && name == "float")
				{
					if(op == "+" || op == "-" || op == "/" || op == "*") return mk_valtype("float");
					else throw ERROR("Unknown operator"); //FIXME
				}
				if(t.name == "int" && name == "float") throw UNIMPL;
				if(t.name == "float" && name == "int") throw UNIMPL;
				throw ERROR("Incompatibile operand types of a binary operator");
			}
			string gen_binop(string op,const type& t,vector<string> args) //DONE
			{
				if(name == "int" && t.name == "int")
				{
					return gen_deref_type()+" "+args[0]+" = ("+gen_deref(args[1])+op+t.gen_deref(args[2])+")";
				}
				if(name == "float" && t.name == "float")
				{
					return gen_deref_type()+" "+args[0]+" = ("+gen_deref(args[1])+op+t.gen_deref(args[2])+")";
				}
				if(name == "int" && t.name == "float")
				{
					throw UNIMPL; // FIXME
				}
				if(name == "float" && t.name == "int")
				{
					throw UNIMPL; // FIXME
				}
				throw WTF;
			}
			type check_assign(const type& t)
			{
				if(is_ref || is_varref)
				{
					if(t.name == "int" && name == "int") return (*this);
					if(t.name == "float" && name == "float") return (*this);
					throw ERROR("only primitive types have builtin assignment");
				}
				throw ERROR("left operand of an assignment has to be a l-value");
			}
			string gen_assign(const type& t, vector<string> args) //DONE
			{
				return gen_deref(args[0])+" = "+t.gen_deref(args[1]);
			}
			type check_index(const type& t)
			{
				if(name != "vector") throw D(name),ERROR("Index operator is only for vectors");
				if(t.name != "int") throw ERROR("Only ints can serve as indices");
				return params[0];
			}
			string gen_index(const type& t,vector<string> args)	//DONE
			{
				return "__global void* "+args[0]+" = "+args[1]+" + ("+t.gen_deref(args[2])+"*"+params[0].gen_sizeof()+")";
			}
			type check_copy()
			{
				if(name == "float" || name == "int") return mk_varref(name);
				else throw ERROR("Local vars can be only of type int and float");
			}
			string gen_copy(vector<string> args)	//DONE
			{
				return name+" "+args[0]+" = "+gen_deref(args[1]);
			}
			static type check_getglobalid(int a, vector<string> args)
			{
				if(a>3 || a<0) throw ERROR("opencl::get_global_id(a) -> a has to be one of 0,1,2");
				return mk_varref("int");
			}
			static string gen_getglobalid(int a, vector<string> args)
			{
				return "int "+args[0]+" = get_global_id("+std::to_string(a)+")";
			}
		};
		
		ostream& operator<<(ostream& out,const type& t)
		{
			out<<t.name;
			out<<"(is_ref="<<t.is_ref;
			out<<",is_varref="<<t.is_varref;
			out<<",vec_size="<<t.vec_size.str();
			if(t.params.size()>=1)
			out<<",\nparams[0]="<<t.params[0];
			if(t.params.size()>=2)
			out<<",\nparams[1]="<<t.params[1];
			out<<")";
			return out;
		}
		
		class globvar		// klasa ogarniająca przesyłanie danych między cpu a gpu
		{
			type t;
			void* ptr;
			id_t name;
			// TODO: ogarnąć więcej niż minimum
			public:
			globvar():ptr(NULL){}
			globvar(id_t nm, vector<int>& v)
			{
				name = nm;
				t = type::mk_vectype(type::mk_reftype("int"),id_t::generate("visize"));
				ptr = &v;
			}
			globvar(id_t nm, vector<float>& v)
			{
				name = nm;
				t = type::mk_vectype(type::mk_reftype("float"),id_t::generate("vfsize"));
				ptr = &v;
			}
			globvar(id_t nm, int& a)
			{
				name = nm;
				ptr = &a;
				t = type::mk_valtype("int");
			}
			globvar(id_t nm, float& a)
			{
				name = nm;
				ptr = &a;
				t = type::mk_valtype("float");
			}
			
			template<typename T> globvar(id_t nm, vector<T>& v)
			{
				//TODO
				throw UNIMPL;
			}
			const type& get_type()
			{
				return t;
			}
			const id_t& get_name()
			{
				return name;
			}
			void send(api_abstr::scope& s)
			{
				if(t.name == "vector")
				if(t.params[0].name == "int" || t.params[0].name == "float")
				{
					vector<int>& v = *(vector<int>*)(ptr);
					void* bufptr = &(v[0]);
					s.send_buf(name.str(),bufptr,v.size()*4);
					s.send_int(t.vec_size.str(),v.size());
					return;
				}
				if(t.name == "int")
				{
					s.send_int(name.str(),*(int*)ptr);
					return;
				}
				if(t.name == "float")
				{
					s.send_float(name.str(),*(float*)ptr);
					return;
				}
				throw UNIMPL;
			}
			void fetch(api_abstr::scope& s)
			{
				if(t.name == "vector")
				if(t.params[0].name == "int" || t.params[0].name == "float")
				{
					vector<int>& v = *(vector<int>*)(ptr);
					void* bufptr = &(v[0]);
					s.fetch_buf(name.str(),bufptr);
					return;
				}
				if(t.name == "int" || t.name == "float") return;
				throw UNIMPL;
			}
			void declare(api_abstr::scope& s)
			{
				if(t.name == "vector")
				{
					s.decl_buf(name.str());
					s.decl_int(t.vec_size.str());
				}
				if(t.name == "int")
				s.decl_int(name.str());
				if(t.name == "float")
				s.decl_float(name.str());
			}
		};
		
		class env
		{
			instr_stack outcode;
			map<id_t,type> ctx;
			map<void*,globvar> globals;// to, co trzeba przenosić między cpu a gpu
			public:
			
			// dane z cpu:
			id_t eat(vector<int>& v)
			{
				if(globals.count(&v)) return globals[&v].get_name();
				id_t nm = id_t::generate("vi");
				globals[&v] = globvar(nm,v);
				ctx[nm] = globals[&v].get_type();
				D(ctx[nm]);
				D("!!!!!!!!");
				return nm;
			}
			id_t eat(vector<float>& v)
			{
				if(globals.count(&v)) return globals[&v].get_name();
				id_t nm = id_t::generate("vf");
				globals[&v] = globvar(nm,v);
				ctx[nm] = globals[&v].get_type();
				return nm;
			}
			id_t eat(int& x)
			{
				if(globals.count(&x)) return globals[&x].get_name();
				id_t nm = id_t::generate("i");
				globals[&x] = globvar(nm,x);
				ctx[nm] = globals[&x].get_type();
				return nm;
			}
			id_t eat(float& x)
			{
				if(globals.count(&x)) return globals[&x].get_name();
				id_t nm = id_t::generate("f");
				globals[&x] = globvar(nm,x);
				ctx[nm] = globals[&x].get_type();
				return nm;
			}
			
			// operacje:
			id_t gen_binop(id_t a,string op,id_t b)
			{
				id_t nm = id_t::generate();
				vector<string> args(3);
				args[0] = nm.str();
				args[1] = a.str();
				args[2] = b.str();
				ctx[nm] = ctx[a].check_binop(op,ctx[b]);
				string wyn = ctx[a].gen_binop(op,ctx[b],args);
				outcode.add(wyn);
				return nm;
			}
			id_t gen_unop(string op,id_t a)
			{
				id_t nm = id_t::generate();
				vector<string> args(2);
				args[0] = nm.str();
				args[1] = a.str();
				ctx[nm] = ctx[a].check_unop(op);
				string wyn = ctx[a].gen_unop(op,args);
				outcode.add(wyn);
				return nm;
			}
			id_t gen_index(id_t a,id_t b)
			{
				id_t nm = id_t::generate();
				vector<string> args(3);
				args[0] = nm.str();
				args[1] = a.str();
				args[2] = b.str();
				ctx[nm] = ctx[a].check_index(ctx[b]);
				string wyn = ctx[a].gen_index(ctx[b],args);
				outcode.add(wyn);
				return nm;
			}
			id_t gen_assign(id_t a,id_t b)
			{
				vector<string> args(2);
				args[0] = a.str();
				args[1] = b.str();
				string wyn = ctx[a].gen_assign(ctx[b],args);
				outcode.add(wyn);
				return a;
			}
			id_t gen_first(id_t a)
			{
				id_t nm = id_t::generate();
				vector<string> args(2);
				args[0] = nm.str();
				args[1] = a.str();
				ctx[nm] = ctx[a].check_first();
				string wyn = ctx[a].gen_first(args);
				outcode.add(wyn);
				return nm;
			}
			id_t gen_second(id_t a)
			{
				id_t nm = id_t::generate();
				vector<string> args(2);
				args[0] = nm.str();
				args[1] = a.str();
				ctx[nm] = ctx[a].check_second();
				string wyn = ctx[a].gen_second(args);
				outcode.add(wyn);
				return nm;
			}
			id_t gen_copy(id_t a)	// do alokacji nowego rejestru na zmienną 'varref' zwraca jej nazwę
			{
				id_t nm = id_t::generate();
				vector<string> args(2);
				args[0] = nm.str();
				args[1] = a.str();
				ctx[nm] = ctx[a].check_copy();
				string wyn = ctx[a].gen_copy(args);
				outcode.add(wyn);
				return nm;
			}
			id_t gen_getglobalid(int a)	// generuje get_global_id(a) w opencl. zwraca varref
			{
				id_t nm = id_t::generate("g_id");
				vector<string> args(1);
				args[0] = nm.str();
				ctx[nm] = type::check_getglobalid(a,args);
				string wyn = type::gen_getglobalid(a,args);
				outcode.add(wyn);
				return nm;
			}
			
			// struktury kontrolne:
			void gen_if(id_t a)
			{
				outcode.add_if(a.str());
			}
			void gen_else()
			{
				outcode.add_else();
			}
			void gen_for(id_t it, id_t p, id_t q)
			{
				// FIXME FIXME: nie zawsze trzeba robić gen_copy (jak jest to stała, a nie adres, to można ją po prostu wrzucić)
				id_t pp = gen_copy(p);
				id_t qq = gen_copy(q);
				outcode.add_for(it.str(), pp.str(), qq.str());
			}
					//TODO: rozkminić parametry tego
					// parametry fora:
					// - albo for(i,start,end) // i - ref lub varref
					// - albo for(i,v)  // i - j.w. , v - vector
			void open_block(int l_start, int l_end)
			{
				outcode.open(l_start, l_end);
			}
			void close_block()
			{
				outcode.close();
			}
			
			// inne:
			void copyin(api_abstr::scope& s)
			{
				for(auto& p : globals)
				{
					p.second.send(s);
				}
			}
			void copyout(api_abstr::scope& s)
			{
				for(auto& p : globals)
				{
					p.second.fetch(s);
				}
			}
			void declare_deps(api_abstr::scope& s) // deklaruje w obj scope, że dane zmienne globalne istnieją
			{
				// i że trzeba je uwzględnić w generowaniu listy parametrów kernela
				for(auto& p : globals)
				{
					p.second.declare(s);
				}
			}
			string get_code()
			{
				return outcode.get_code();
			}
			
			string get_opened_blocks()
			{
				ostringstream wyn;
				for(int i = int(outcode.blocks.size())-1 ; i>=0 ; --i)
				{
					wyn << "in block [" << outcode.blocks[i].first << " .. " << outcode.blocks[i].second << "]\n";
				}
				
				return wyn.str();
			}
			
		};
	}
}

		
		/*
		TODO LIST:
		= ogarnąć współdzielenie zasobów (zm. globalnych) między kernelami
			(poprzez zrobienie map<void*,globvar> zmienną statyczną i dodanie oprócz niej vector<void*> do env)
		= dodanie copyin dla sumy n kerneli (żeby ogarniało, że każdą zmienną glob. kopiuje tylko raz)
			przez zrobienie kernel.copyin(...) jako va_arg oraz dodanie statycznej funkcji kernel::copyin
			też jako va_arg. to samo z copyout.
		- ogarnąć struktury kontrolne (ify i pętle i breaki)
		- jakieś rzeczy kosmetyczne (więcej operatorów, pozostałe konstr. klasy kernel, itd.)
		- ogarnąć inteligentne śledzenie zależności warunku od pętli while (to będzie najcięższe)
		
		użytkownik będzie widział klasę kernel.
		ta klasa ma metody:
		 - copyin()
		 - copyout()
		 - run(range)
		 - copyandrun(range)
		klasa kernel będzie miała w sobie wskaźnik do klasy env oraz klasy api_abstr::scope
*/

#endif
