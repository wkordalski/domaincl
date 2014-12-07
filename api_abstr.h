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

#ifndef API_ABSTR_H
#define API_ABSTR_H

namespace domaincl
{
	namespace api_abstr
	{
		/*
		tutaj główną jednostką pamięci są jeszcze bajty
			TODO:
			- przetestować - wstępnie działa :)
			- dodać [poprawne, działające] zwalnianie zasobów
			- dodać rzucanie wyjątków i ogólnie obsługę błędów
		*/
		
		class scope
		{
			struct hardware
			{
				bool initialised;
				cl_platform_id platform_id;
    			cl_device_id device_id;
    			cl_context context;
    			cl_command_queue command_queue;
    			public:
    			hardware():initialised(false){}
    			void init()
    			{
    				if(initialised)return;
    				initialised = true;
    				// Get platform and device information
					platform_id = NULL;
					device_id = NULL;
					cl_uint ret_num_devices;
					cl_uint ret_num_platforms;
					cl_int ret = clGetPlatformIDs(1, &platform_id, &ret_num_platforms);
					ret = clGetDeviceIDs( platform_id, CL_DEVICE_TYPE_GPU, 1, &device_id, &ret_num_devices);
					
					// Create an OpenCL context
					context = clCreateContext( NULL, 1, &device_id, NULL, NULL, &ret);
					
					// Create a command queue
					command_queue = clCreateCommandQueue(context, device_id, 0, &ret);
    			}
    			~hardware()
    			{
    				/*
    				int ret;
    				ret = clFlush(command_queue);
    				ret = clFinish(command_queue);
    				ret = clReleaseCommandQueue(command_queue);
    				ret = clReleaseContext(context);
    				*/
    			}
			};
			static hardware hw;
			static string domaincl_kernel_name()
			{
				return "domaincl_kernel";
			}
			
			struct gpu_val
			{
				string name;
				char type; // possible values: 'u' - undefined ; 'b' - buffer ; 'i' - int ; 'f' - float ; 'd' - double
				union
				{
					cl_mem b;
					int i;
					float f;
					double d;
				};
				int bufsize;
				gpu_val():type('u'){}
				gpu_val(string n,cl_mem bb,int s):name(n),type('b'),b(bb),bufsize(s){}
				gpu_val(string n,int ii):name(n),type('i'),i(ii),bufsize(0){}
				gpu_val(string n,float ff):name(n),type('f'),f(ff),bufsize(0){}
				gpu_val(string n,double dd):name(n),type('d'),d(dd),bufsize(0){}
				
				~gpu_val()
				{
					/*
					if(type=='b')
					clReleaseMemObject(b);
					*/
				}
			};
			
			struct gpu_prog
			{
				vector<string> globals;
				vector<string> extents;
				cl_program program;
				cl_kernel kernel;
				
				public:
				~gpu_prog()
				{
					/*
					int ret;
					ret = clReleaseKernel(kernel);
					ret = clReleaseProgram(program);
					*/
				}
			};
			
			map<string,gpu_val> vals;
			map<string,char> decls;
			map<string,gpu_prog> progs;
			
			string get_openclc_decl(string a)
			{
				if(decls[a]=='b') return "__global void* "+a;
				if(decls[a]=='i') return "const int "+a;
				if(decls[a]=='f') return "const float "+a;
				if(decls[a]=='d') return "const double "+a;
			}
			
			void print_compile_error(cl_program& prog,const string& c)
			{
				size_t logSize;
				// check build log
				clGetProgramBuildInfo(prog, hw.device_id,
				CL_PROGRAM_BUILD_LOG, 0, NULL, &logSize);
				char* programLog = (char*) calloc (logSize+1, sizeof(char));
				clGetProgramBuildInfo(prog, hw.device_id,
				CL_PROGRAM_BUILD_LOG, logSize+1, programLog, NULL);
				cerr<<"\t[OpenCL C code]"<<endl;
				cerr<<c<<endl;
				cerr<<"\t[OpenCL C compilation log]"<<endl;
				cerr<<programLog<<endl;
				free(programLog);
			}
			
			public:
			
			scope(string dev_type)		//dev_type == CPU | GPU FIXME TODO
			{
				hw.init();
			}
			~scope()
			{
				//TODO
			}
			
			void decl_buf(string name){ decls[name] = 'b'; }
			void decl_int(string name){ decls[name] = 'i'; }
			void decl_float(string name){ decls[name] = 'f'; }
			void decl_double(string name){ decls[name] = 'd'; }
			
			void send_buf(string name, void* ptr, int size)
			{
				decl_buf(name);
				int ret;
				cl_mem b_mem = clCreateBuffer(hw.context, CL_MEM_READ_WRITE, size, NULL, &ret);
				ret = clEnqueueWriteBuffer(hw.command_queue, b_mem, CL_TRUE, 0, size, ptr, 0, NULL, NULL);
				vals[name] = gpu_val(name,b_mem,size);
			}
			void send_int(string name, int a)
			{
				decl_int(name);
				vals[name] = gpu_val(name,a);
			}
			void send_float(string name, float a)
			{
				decl_int(name);
				vals[name] = gpu_val(name,a);
			}
			void send_double(string name, double a)
			{
				decl_int(name);
				vals[name] = gpu_val(name,a);
			}
			
			void alloc_buf(string name, int size)
			{
				decl_buf(name);
				int ret;
				cl_mem b_mem = clCreateBuffer(hw.context, CL_MEM_READ_WRITE, size, NULL, &ret);
				vals[name] = gpu_val(name,b_mem,size);
			}
			
			void* fetch_buf(string name, void* ptr=NULL)
			{
				if(ptr==NULL) ptr = malloc(vals[name].bufsize);
				int ret;
				ret = clEnqueueReadBuffer(hw.command_queue, vals[name].b, CL_TRUE, 0, 
				vals[name].bufsize, ptr, 0, NULL, NULL);
				return ptr;
			}
			int fetch_buf_size(string name)
			{
				return vals[name].bufsize;
			}
			int fetch_int(string name){return vals[name].i;}		//FIXME: bez sensu
			float fetch_float(string name){return vals[name].f;}//FIXME: bez sensu
			double fetch_double(string name){return vals[name].d;}//FIXME: bez sensu
			
			void compile_prog(string name,vector<pair<string,string> > loopheader, string code_in)
			{
				if(loopheader.size()>3)throw error("loopheader.size() [work dimensions] > 3 is forbidden by the OpenCL standard!");
				gpu_prog p;
				// generating opencl c code:
				string code_out="";
				for(auto& i : decls)
				{
					p.globals.push_back(i.first);
				}
				code_out += "__kernel void "+domaincl_kernel_name()+"(";
				for(auto& s : p.globals)
				{
					code_out += get_openclc_decl(s);
					code_out += ",";
				}
				if(p.globals.size())code_out.erase(code_out.size()-1);
				code_out += "){\n";
				for(int i=0;i<loopheader.size();++i)
				code_out += "int "+loopheader[i].first+" = get_global_id("+to_string(i)+");\n";
				code_out += code_in;
				code_out += "}";
				// some boilerplate:
				int ret;
				size_t source_size = code_out.size();
				const char * source_ptr = code_out.c_str();
				p.program = clCreateProgramWithSource(hw.context, 1, 
				(const char **)(&source_ptr), (const size_t *)&source_size, &ret);
				ret = clBuildProgram(p.program, 1, &(hw.device_id), NULL, NULL, NULL);
				if(ret != CL_SUCCESS)
				{
					print_compile_error(p.program,code_out);
					throw ERROR("error during clBuildProgram");
				}
				p.kernel = clCreateKernel(p.program, domaincl_kernel_name().c_str(), &ret);
				for(int i=0;i<loopheader.size();++i)
				p.extents.push_back(loopheader[i].second);
				progs[name] = p;
			}
			void compile_prog(string name,pair<string,string> _loopheader, string code_in)
			{
				vector<pair<string,string> > loopheader(1,_loopheader);
				compile_prog(name, loopheader, code_in);
			}
			void compile_prog(string name, vector<string> loopheader, string code_in)
			{
				if(loopheader.size()>3)throw error("loopheader.size() [work dimensions] > 3 is forbidden by the OpenCL standard!");
				gpu_prog p;
				// generating opencl c code:
				string code_out="";
				for(auto& i : decls)
				{
					p.globals.push_back(i.first);
				}
				code_out += "__kernel void "+domaincl_kernel_name()+"(";
				for(auto& s : p.globals)
				{
					code_out += get_openclc_decl(s);
					code_out += ",";
				}
				if(p.globals.size())code_out.erase(code_out.size()-1);
				code_out += "){\n";
				// tu coś było, co zostało usunięte na potrzeby późniejszego kodu
				code_out += code_in;
				code_out += "}";
				// some boilerplate:
				int ret;
				size_t source_size = code_out.size();
				const char * source_ptr = code_out.c_str();
				p.program = clCreateProgramWithSource(hw.context, 1, 
				(const char **)(&source_ptr), (const size_t *)&source_size, &ret);
				ret = clBuildProgram(p.program, 1, &(hw.device_id), NULL, NULL, NULL);
				if(ret != CL_SUCCESS || true) // FIXME FIXME
				{
					print_compile_error(p.program,code_out);
					//throw ERROR("error during clBuildProgram");
				}
				p.kernel = clCreateKernel(p.program, domaincl_kernel_name().c_str(), &ret);
				for(int i=0;i<loopheader.size();++i)
				p.extents.push_back(loopheader[i]);
				progs[name] = p;
			}
			void run_prog(string name)
			{
				gpu_prog& p = progs[name];
				int ret;
				for(int i=0;i<p.globals.size();++i)
				{
					gpu_val& v = vals[p.globals[i]];
					if(v.type == 'b')
						ret = clSetKernelArg(p.kernel, i, sizeof(cl_mem), (void *)&(v.b));
					else if(v.type == 'i')
						ret = clSetKernelArg(p.kernel, i, sizeof(int), (void *)&(v.i));
					else if(v.type == 'f')
						ret = clSetKernelArg(p.kernel, i, sizeof(float), (void *)&(v.f));
					else if(v.type == 'd')
						ret = clSetKernelArg(p.kernel, i, sizeof(double), (void *)&(v.d));
				}
				
				vector<size_t> global_work_size;
				for(int i=0;i<p.extents.size();++i)
				global_work_size.push_back(size_t(vals[p.extents[i]].i));
				
				ret = clEnqueueNDRangeKernel (/*cl_command_queue command_queue*/ hw.command_queue,
				/*cl_kernel kernel*/ p.kernel,
				/*cl_uint work_dim*/ cl_uint(p.extents.size()),
				/*const size_t *global_work_offset*/ NULL,
				/*const size_t *global_work_size*/ &(global_work_size[0]),
				/*const size_t *local_work_size*/ NULL,
				/*cl_uint num_events_in_wait_list*/ 0,
				/*const cl_event *event_wait_list*/ NULL,
				/*cl_event *event*/ NULL);
				
			}
		};
		scope::hardware scope::hw = scope::hardware();
		
	}
	
}

#endif
