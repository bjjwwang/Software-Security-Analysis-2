//===- AEMgr.cpp -- Abstract Execution Manual Translations---------------------------------//
//
//                     SVF: Static Value-Flow Analysis
//
// Copyright (C) <2013->  <Yulei Sui>
//

// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU Affero General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.

// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU Affero General Public License for more details.

// You should have received a copy of the GNU Affero General Public License
// along with this program.  If not, see <http://www.gnu.org/licenses/>.
//
//===----------------------------------------------------------------------===//

//
// Created on 2024/1/10.
//

#include "AEMgr.h"

namespace SVF {
	/*
	// A simple example

	int main() {
		int* p;
		int q;
		int* r;
		int x;

		p = malloc1(..);
		q = 5;
		*p = q;
		x = *p;
		assert(x==10);
	}
	*/
	void AbstractExecutionMgr::test0(){

		//  int** p;
		NodeID p = getNodeID("p");

		//  int q;
		NodeID q = getNodeID("q");

		//  int* r;
		NodeID r = getNodeID("r");

		//  int x;
		NodeID x = getNodeID("x");

		// p = malloc(..);
		NodeID malloc = getNodeID("malloc");
		as[p] = AddressValue(getMemObjAddress("malloc"));

		// q = 5;
		as[q] = IntervalValue(5, 5);

		// *p = q;
		as.storeValue(p, as[q]);

		// x = *p;
		as[x] = as.loadValue(p);
	}

	//    int main() {
	//        int a;
	//        int b;
	//        a = 0;
	//        b = a + 1;
	//        assert(b>0);
	//    }
	void AbstractExecutionMgr::test1() {
		NodeID a = getNodeID("a");
		NodeID b = getNodeID("b");
		/// TODO: your code starts from here

	}

	//        int main() {
	//        int* p;
	//        int q;
	//        int b;
	//        int a;
	//        p = &a;
	//        *p = 0;
	//        q = *p;
	//        *p = 3;
	//        b = *p + 1;
	//        assert(b>3);
	//    }
	void AbstractExecutionMgr::test2() {
		NodeID p = getNodeID("p");
		NodeID q = getNodeID("q");
		NodeID b = getNodeID("b");
		/// TODO: your code starts from here

	}

	//    int main() {
	//        int** p;
	//        int* q;
	//        int* r;
	//        int x;
	//
	//        p = malloc1(..);
	//        q = malloc2(..);
	//        *p = q; // storeAddr
	//        *q = 10; // storeValue
	//        r = *p;
	//        x = *r;
	//        assert(x==10);
	//    }
	void AbstractExecutionMgr::test3() {
		NodeID p = getNodeID("p");
		NodeID q = getNodeID("q");
		NodeID r = getNodeID("r");
		NodeID x = getNodeID("x");
		/// TODO: your code starts from here

	}

	//        int* p;
	//        int* x;
	//        int* y;
	//        int a;
	//        int b;
	//        p = malloc;
	//        x = &p[0];
	//        y = &p[1]
	//        *x = 10;
	//        *y = 11;
	//        a = *x;
	//        b = *y;
	//        assert((a + b)>20);
	void AbstractExecutionMgr::test4() { //    int main() {
		NodeID p = getNodeID("p");
		NodeID x = getNodeID("x");
		NodeID y = getNodeID("y");
		NodeID a = getNodeID("a");
		NodeID b = getNodeID("b");
		/// TODO: your code starts from here

	}

	//// Struct and pointers
	//
	//    struct A{ int f0; int* f1;};
	//    int main() {
	//       struct A* p;
	//       int a;
	//       int* x;
	//       int* q;
	//       int** r;
	//       int* y;
	//       int z;
	//
	//       p = malloc;
	//       x = &a;
	//       *x = 5;
	//       q = &(p->f0);
	//       *q = 10;
	//       r = &(p->f1);
	//       *r = x;
	//       y = *r;
	//       z = *q + *y;
	//       assert(z==15);
	void AbstractExecutionMgr::test5() {
		NodeID p = getNodeID("p", 2);
		NodeID a = getNodeID("a");
		NodeID x = getNodeID("x");
		NodeID q = getNodeID("q");
		NodeID r = getNodeID("r");
		NodeID y = getNodeID("y");
		NodeID z = getNodeID("z");

	}

	//    int main(int argv) {  // argv is an interval  [4, 10]
	//    int a;
	//    int b;
	//    a = argv + 1;
	//    b = 5;
	//    if(a > 10)
	//       b = a;
	//    assert(b>=5);
	//    }
	void AbstractExecutionMgr::test6() {
		NodeID a = getNodeID("a");
		NodeID b = getNodeID("b");
		NodeID argv = getNodeID("argv");
		/// TODO: your code starts from here

	}

	// int foo(int z) {
	//     k = z;
	//     return k;
	// }
	// int main() {
	//   int x;
	//   int y;
	//   y = foo(2);
	//   x = foo(3);
	//   assert(x== 3 && y==2);
	// }
	void AbstractExecutionMgr::test7() {
		NodeID x = getNodeID("x");
		NodeID y = getNodeID("y");
		/// TODO: your code starts from here


	}

	// int main() {
	//    int x;
	//    x=20;
	//    while(x>0) {
	//        x--;
	//    }
	//    assert(x == 0);
	//    return 0;
	// }
	void AbstractExecutionMgr::test8() {
		NodeID x = getNodeID("x");
		/// TODO: your code starts from here

	}

} // namespace SVF