domaincl
========

An OpenCL wrapper - domain specific language embedded in C++ for GPGPU purposes.
This is an early prorotype, however it already works on small programs.
It provides types 'val' and 'var' wchich are essentially
nodes of the AST. User writes a kernel that is operating on these types.
The library takes this kernel, applies it to object of type 'val' and as a result gets
the AST of that kernel. Then it transforms it to the OpenCL C and runs.
See usage examples for more info.

dependencies:
-------------
You need to have OpenCL installed on your computer and a C++ compiler
wchich supports C++11 (it compiles under gcc 4.7.2 for example).

Usage examples:
---------------

#### vector addition

```c++
int main()
{
	vector<float> a,b,c;
	a.resize(100,0);
	b.resize(100,0);
	c.resize(100,0);
	/* ...some initialisation here... */
	function<void(val)> f = [&](val i)
	{
		val _a = a;
		val _b = b;
		val _c = c;
		_c[i] = _a[i] + _b[i]
	};
	kernel k = f;
	k.copyin();
	k.run(100);
	k.copyout();
	for(int i=0;i<100;++i)
	cout << c[i] << " ";
}
```

#### Pi approximation

```c++
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
```

