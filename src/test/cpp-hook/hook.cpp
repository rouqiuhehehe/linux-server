// //
// // Created by 115282 on 2023/8/17.
// //
// #include <iostream>
// class A
// {
// public:
//     void b ()
//     {
//         std::cout << "bbb" << std::endl;
//     }
// };
//
// void newB(A *a) {
//     std::cout << "new bbbb" << std::endl;
//
//     a->b();
// }
// void c() {
//     void (A::*oldBb)() = &A::b;
//
//     *reinterpret_cast<void (A::*)()>(&A::b) = &newB;
// }