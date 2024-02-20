
#define OPT 

extern "C" {

int ng_test_arg;
int ng_test_args0(void) {
#ifdef OPT
  // avoid optimization
  ng_test_arg=ng_test_arg+ng_test_arg+ng_test_arg+ng_test_arg+ng_test_arg
              +ng_test_arg+ng_test_arg+ng_test_arg+ng_test_arg+ng_test_arg
              +ng_test_arg;
  return ng_test_arg;
#else
  return 0;
#endif
}

int ng_test_args1(int *arg1) {
#ifdef OPT
  // avoid optimization
  *arg1 = *arg1+*arg1+*arg1+*arg1+*arg1+*arg1+*arg1+*arg1+*arg1+*arg1+*arg1;
  return *arg1;
#else
  return 0;
#endif
}

int ng_test_args2(int *arg1, int *arg2) {
#ifdef OPT
  // avoid optimization
  *arg1 = *arg2+*arg1+*arg1+*arg1+*arg2+*arg2+*arg2+*arg2+*arg1+*arg1+*arg2;
  return *arg1;
#else
  return 0;
#endif
}

int ng_test_args3(int *arg1, int *arg2, int *arg3) {
#ifdef OPT
  // avoid optimization
  *arg1 = *arg2+*arg3+*arg1+*arg1+*arg2+*arg2+*arg3+*arg2+*arg3+*arg1+*arg2;
  return *arg1;
#else
  return 0;
#endif
}

int ng_test_args4(int *arg1, int *arg2, int *arg3, int *arg4) {
#ifdef OPT
  // avoid optimization
  *arg1 = *arg2+*arg3+*arg4+*arg1+*arg2+*arg2+*arg3+*arg4+*arg3+*arg1+*arg2;
  return *arg1;
#else
  return 0;
#endif
}

int ng_test_args5(int *arg1, int *arg2, int *arg3, int *arg4, int *arg5) {
#ifdef OPT
  // avoid optimization
  *arg1 = *arg2+*arg3+*arg4+*arg5+*arg5+*arg5+*arg5+*arg4+*arg3+*arg1+*arg2;
  return *arg1;
#else
  return 0;
#endif
}

int ng_test_args6(int *arg1, int *arg2, int *arg3, int *arg4, int *arg5, int *arg6) {
#ifdef OPT
  // avoid optimization
  *arg1 = *arg2+*arg3+*arg4+*arg5+*arg6+*arg6+*arg5+*arg4+*arg3+*arg1+*arg2;
  return *arg1;
#else
  return 0;
#endif
}

int ng_test_args7(int *arg1, int *arg2, int *arg3, int *arg4, int *arg5, int *arg6, 
          int *arg7) {
#ifdef OPT
  // avoid optimization
  *arg1 = *arg2+*arg3+*arg4+*arg5+*arg6+*arg7+*arg5+*arg4+*arg3+*arg1+*arg2;
  return *arg1;
#else
  return 0;
#endif
}

int ng_test_args8(int *arg1, int *arg2, int *arg3, int *arg4, int *arg5, int *arg6, 
          int *arg7, int *arg8) {
#ifdef OPT
  // avoid optimization
  *arg1 = *arg2+*arg3+*arg4+*arg5+*arg6+*arg7+*arg8+*arg4+*arg3+*arg1+*arg2;
  return *arg1;
#else
  return 0;
#endif
}

int ng_test_args9(int *arg1, int *arg2, int *arg3, int *arg4, int *arg5, int *arg6, 
          int *arg7, int *arg8, int *arg9) {
#ifdef OPT
  // avoid optimization
  *arg1 = *arg2+*arg3+*arg4+*arg5+*arg6+*arg7+*arg8+*arg9+*arg3+*arg1+*arg2;
  return *arg1;
#else
  return 0;
#endif
}

int ng_test_args10(int *arg1, int *arg2, int *arg3, int *arg4, int *arg5, int *arg6, 
          int *arg7, int *arg8, int *arg9, int *arg10) {
#ifdef OPT
  // avoid optimization
  *arg1 = *arg2+*arg3+*arg4+*arg5+*arg6+*arg7+*arg8+*arg9+*arg10+*arg1+*arg2;
  return *arg1;
#else
  return 0;
#endif
}

int ng_test_args11(int *arg1, int *arg2, int *arg3, int *arg4, int *arg5, int *arg6, 
          int *arg7, int *arg8, int *arg9, int *arg10, int *arg11) {
#ifdef OPT
  // avoid optimization
  *arg1 = *arg2+*arg3+*arg4+*arg5+*arg6+*arg7+*arg8+*arg9+*arg10+*arg11+*arg1;
  return *arg1;
#else
  return 0;
#endif
}

int ng_test_args12(int *arg1, int *arg2, int *arg3, int *arg4, int *arg5, int *arg6, 
          int *arg7, int *arg8, int *arg9, int *arg10, int *arg11, int *arg12) {
#ifdef OPT
  // avoid optimization
  *arg1 = *arg2+*arg3+*arg4+*arg5+*arg6+*arg7+*arg8+*arg9+*arg10+*arg11+*arg12;
  return *arg1;
#else
  return 0;
#endif
}

int ng_test_pointers(int *arg1, int *arg2, int *arg3) {
#ifdef OPT
  // avoid optimization
  int arg = *arg1+*arg2+*arg3;
  return arg;
#else
  return 0;
#endif
}

int ng_test_direct(int arg1, int arg2, int arg3) {
#ifdef OPT
  // avoid optimization
  arg1 = arg1+arg2+arg3;
  return arg1;
#else
  return 0;
#endif
}


}
