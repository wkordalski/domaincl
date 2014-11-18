domaincl
========

An OpenCL wrapper - DSL embedded in C++ for GPGPU purposes.

Usage examples
---------------

vector addition
----------------
```
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

Pi approximation
-----------------
```
int get_random()
{
	/* some RNG */
}

val next_rand(val a)
{
	/* some PRNG */
}

const int MAX_NEXT_RAND;

int main()
{
	vector<int> rands(100,0);
	vector<int> cnts(100,0);
	
	for(int i=0;i<100;++i)
	rands[i] = get_random();
	
	function<void(val)> f = [&](val i)
	{
		val _rands = rands;
		val wyn = cnts;
		val r = _rands[i];
		For(j,0,500) DO(
			val x = r;
			r = next_rand(r);
			val y = r;
			If(x*x+y*y <= MAX_NEXT_RAND*MAX_NEXT_RAND) DO(
				wyn[i] += 1;
			)
			r = next_rand(r);
		)
	};
	
	kernel k = f;
	k.copyin();
	k.run(100);
	k.copyout();
	
	int cnt = 0;
	for(int i=0;i<100;++i)
	cnt += cnts[i];
	
	cout << " Pi = " << double(cnt)/(100.0*500.0) << endl;
}
```
