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

namespace domaincl_examples
{

	using namespace dsl;

	void vector_add()
	{
		vector<int> a,b,c;
		auto func = [&](val i)
		{
			//import the data:
			val _a = a;
			val _b = b;
			val _c = c;
			//compute:
			_c[i] = _a[i] + _b[i];
		};
		//compile the kernel:
		kernel k = function<void(val)>(func);
		//some initialisation:
		a.resize(10,0);
		b.resize(10,0);
		c.resize(10,0);
		for(int i=0;i<10;++i)
		{
			a[i] = i;
			b[i] = i;
		}
		//move the data and run:
		k.copyin();
		k.run(10);
		k.copyout();
		for(int i=0;i<10;++i)
		cout<<c[i]<<" ";
		cout<<endl;
	}
	
	const int MAX_NEXT_RAND = 1007;
	
	int get_random()
	{
	    /* some RNG */
	    return rand() % MAX_NEXT_RAND;
	}

	val next_rand(val a)
	{
		/* some PRNG */
		int d = 10;
		var wyn = 0;
		while(d--)
		{
			wyn = wyn * 4;
			wyn = wyn + a % 4;
			a = a*a % MAX_NEXT_RAND;
		}
		return wyn % MAX_NEXT_RAND;
	}
	
	void pi_approximation()
	{
	    vector<int> rands(100,0);
	    vector<int> cnts(100,0);
	
	    for(int i=0;i<100;++i)
	    rands[i] = get_random();
	
	    function<void(val)> f = [&](val i)
	    {
	        val _rands = rands;
	        val _cnts = cnts;
	        var r = _rands[i];
	        var j = val(0);
	        For(j,1,1000) DO(
					
	            var x = next_rand(r);
	            var y = next_rand(r);
	            
	            If(x*x+y*y <= MAX_NEXT_RAND*MAX_NEXT_RAND) DO(
	                _cnts[i] = _cnts[i] + 1;
	            )
	        )
	    };
	
	    kernel k = f;
	    k.copyin();
	    k.run(100);
	    k.copyout();
		
		
		 //cout<<"cnts = ";
	    int cnt = 0;
	    for(int i=0;i<100;++i)
	    {
	    	cnt += cnts[i];
	    	//cout<<cnts[i]<<" ";
	    }
	    //cout<<endl;
	
	    cout << " Pi = " << 4 * double(cnt)/(100.0*1000.0) << endl;
	}

}

int main()
{
	domaincl_examples::pi_approximation();
	return 0;
}
