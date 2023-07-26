class Main {
	aaa: Bool <- false;
	b: Bool <- aaa;
	c: Bool <- c;
	init(a: Int, b: Bool, c: String): Main {
		{
		a <- 42;
		new SELF_TYPE;
		}
	};
	main(): Int {
		42
	};
	testAgain(a: Int, b: Bool, c: String): SELF_TYPE {
		new SELF_TYPE
	};
};

class A inherits Main {
	p: Penis;
	init(a: Int, b: Bool, c: String): Main {
		{
			p <- new Penis;
			new A;	
		}
	};
};

class Penis inherits A {
	pp: Object;
	bbb: Int;
	ccc: Main;
	xxx: Int;
	z: Bool <- true;
	asdasdk: Bool <- not false;
	test(): Bool {
		{
			aaa <- true;
			p <- new Penis;
			pp <- testAgain(45, true, "hello");
			ccc <- if true then new Penis else new Penis fi;
			pp <- let x: Int <- 42, y: Int <- 42, z: SELF_TYPE <- new SELF_TYPE in z;
			pp <- case new Main of yyy : Main => yyy; ooo : Object => ooo; esac;
			pp <- while true loop 42 pool;
			true;
		}
	};

};
