#include <gio/gio.h>

#if defined (__ELF__) && ( __GNUC__ > 2 || (__GNUC__ == 2 && __GNUC_MINOR__ >= 6))
# define SECTION __attribute__ ((section (".gresource.wayward"), aligned (8)))
#else
# define SECTION
#endif

#ifdef _MSC_VER
static const SECTION union { const guint8 data[861]; const double alignment; void * const ptr;}  wayward_resource_data = { {
  0107, 0126, 0141, 0162, 0151, 0141, 0156, 0164, 0000, 0000, 0000, 0000, 0000, 0000, 0000, 0000, 
  0030, 0000, 0000, 0000, 0254, 0000, 0000, 0000, 0000, 0000, 0000, 0050, 0005, 0000, 0000, 0000, 
  0000, 0000, 0000, 0000, 0003, 0000, 0000, 0000, 0004, 0000, 0000, 0000, 0004, 0000, 0000, 0000, 
  0004, 0000, 0000, 0000, 0210, 0131, 0114, 0047, 0003, 0000, 0000, 0000, 0254, 0000, 0000, 0000, 
  0010, 0000, 0114, 0000, 0264, 0000, 0000, 0000, 0270, 0000, 0000, 0000, 0113, 0120, 0220, 0013, 
  0002, 0000, 0000, 0000, 0270, 0000, 0000, 0000, 0004, 0000, 0114, 0000, 0274, 0000, 0000, 0000, 
  0300, 0000, 0000, 0000, 0324, 0265, 0002, 0000, 0377, 0377, 0377, 0377, 0300, 0000, 0000, 0000, 
  0001, 0000, 0114, 0000, 0304, 0000, 0000, 0000, 0310, 0000, 0000, 0000, 0372, 0112, 0320, 0174, 
  0001, 0000, 0000, 0000, 0310, 0000, 0000, 0000, 0015, 0000, 0114, 0000, 0330, 0000, 0000, 0000, 
  0334, 0000, 0000, 0000, 0320, 0077, 0013, 0266, 0000, 0000, 0000, 0000, 0334, 0000, 0000, 0000, 
  0011, 0000, 0166, 0000, 0350, 0000, 0000, 0000, 0134, 0003, 0000, 0000, 0167, 0141, 0171, 0167, 
  0141, 0162, 0144, 0057, 0004, 0000, 0000, 0000, 0157, 0162, 0147, 0057, 0003, 0000, 0000, 0000, 
  0057, 0000, 0000, 0000, 0001, 0000, 0000, 0000, 0162, 0141, 0163, 0160, 0142, 0145, 0162, 0162, 
  0171, 0055, 0160, 0151, 0057, 0000, 0000, 0000, 0000, 0000, 0000, 0000, 0163, 0164, 0171, 0154, 
  0145, 0056, 0143, 0163, 0163, 0000, 0000, 0000, 0266, 0015, 0000, 0000, 0001, 0000, 0000, 0000, 
  0170, 0332, 0305, 0227, 0341, 0156, 0233, 0060, 0020, 0307, 0277, 0363, 0024, 0247, 0356, 0113, 
  0033, 0101, 0233, 0264, 0123, 0272, 0302, 0267, 0165, 0332, 0036, 0140, 0117, 0140, 0260, 0103, 
  0254, 0030, 0214, 0154, 0223, 0214, 0115, 0175, 0367, 0141, 0114, 0122, 0050, 0306, 0141, 0133, 
  0350, 0260, 0204, 0224, 0263, 0357, 0376, 0347, 0237, 0317, 0066, 0271, 0133, 0170, 0000, 0013, 
  0170, 0346, 0105, 0045, 0150, 0272, 0125, 0160, 0375, 0174, 0003, 0367, 0313, 0325, 0143, 0120, 
  0277, 0236, 0140, 0217, 0262, 0122, 0170, 0213, 0073, 0317, 0273, 0075, 0240, 0352, 0200, 0004, 
  0016, 0012, 0224, 0023, 0006, 0277, 0152, 0067, 0200, 0030, 0045, 0273, 0124, 0360, 0062, 0307, 
  0101, 0302, 0031, 0027, 0041, 0304, 0254, 0066, 0105, 0336, 0213, 0327, 0361, 0240, 0230, 0221, 
  0306, 0141, 0164, 0070, 0100, 0373, 0073, 0025, 0250, 0152, 0274, 0173, 0316, 0001, 0112, 0024, 
  0335, 0117, 0215, 0161, 0330, 0122, 0105, 0042, 0200, 0176, 0022, 0262, 0222, 0212, 0144, 0243, 
  0171, 0177, 0130, 0143, 0335, 0242, 0261, 0356, 0145, 0363, 0014, 0272, 0151, 0206, 0122, 0022, 
  0102, 0316, 0163, 0142, 0372, 0012, 0204, 0061, 0315, 0323, 0020, 0126, 0017, 0305, 0017, 0143, 
  0072, 0206, 0330, 0044, 0272, 0265, 0041, 0270, 0300, 0104, 0004, 0122, 0125, 0254, 0347, 0336, 
  0332, 0007, 0141, 0133, 0373, 0201, 0142, 0265, 0015, 0141, 0171, 0014, 0335, 0232, 0005, 0302, 
  0264, 0224, 0306, 0016, 0075, 0172, 0146, 0326, 0341, 0226, 0357, 0211, 0030, 0235, 0073, 0142, 
  0305, 0026, 0135, 0267, 0004, 0174, 0130, 0336, 0176, 0272, 0351, 0145, 0156, 0200, 0066, 0070, 
  0273, 0104, 0121, 0211, 0051, 0037, 0007, 0032, 0177, 0324, 0055, 0162, 0152, 0066, 0113, 0247, 
  0025, 0037, 0133, 0105, 0047, 0133, 0033, 0110, 0013, 0357, 0367, 0142, 0333, 0314, 0177, 0032, 
  0132, 0303, 0302, 0211, 0366, 0024, 0066, 0056, 0225, 0342, 0371, 0070, 0327, 0156, 0045, 0036, 
  0155, 0137, 0233, 0347, 0277, 0023, 0061, 0251, 0137, 0036, 0011, 0052, 0012, 0331, 0306, 0073, 
  0125, 0027, 0321, 0155, 0070, 0354, 0214, 0170, 0033, 0334, 0055, 0227, 0060, 0236, 0354, 0306, 
  0027, 0340, 0351, 0136, 0267, 0350, 0017, 0326, 0307, 0242, 0221, 0012, 0212, 0215, 0304, 0331, 
  0275, 0261, 0066, 0174, 0034, 0007, 0337, 0233, 0235, 0363, 0266, 0307, 0233, 0267, 0004, 0214, 
  0200, 0176, 0145, 0110, 0244, 0064, 0017, 0142, 0136, 0227, 0101, 0026, 0302, 0203, 0251, 0220, 
  0123, 0111, 0366, 0073, 0006, 0070, 0202, 0032, 0122, 0346, 0103, 0037, 0321, 0067, 0265, 0373, 
  0174, 0334, 0020, 0356, 0053, 0307, 0301, 0140, 0302, 0241, 0322, 0051, 0214, 0015, 0317, 0125, 
  0260, 0101, 0031, 0145, 0125, 0010, 0127, 0137, 0004, 0257, 0323, 0370, 0216, 0162, 0171, 0325, 
  0351, 0226, 0364, 0147, 0035, 0145, 0265, 0236, 0177, 0207, 0171, 0103, 0116, 0272, 0302, 0031, 
  0252, 0314, 0165, 0150, 0025, 0266, 0313, 0132, 0105, 0155, 0222, 0116, 0312, 0343, 0060, 0055, 
  0364, 0165, 0356, 0346, 0124, 0360, 0173, 0113, 0071, 0123, 0342, 0265, 0334, 0073, 0250, 0000, 
  0360, 0122, 0061, 0232, 0237, 0174, 0255, 0225, 0154, 0302, 0317, 0072, 0127, 0233, 0152, 0347, 
  0372, 0370, 0353, 0165, 0264, 0244, 0173, 0201, 0102, 0206, 0361, 0203, 0352, 0305, 0163, 0115, 
  0306, 0234, 0351, 0076, 0070, 0106, 0230, 0017, 0104, 0337, 0061, 0142, 0303, 0223, 0122, 0272, 
  0006, 0350, 0264, 0260, 0340, 0205, 0153, 0214, 0044, 0214, 0044, 0212, 0140, 0157, 0344, 0123, 
  0364, 0164, 0212, 0050, 0121, 0037, 0030, 0124, 0321, 0332, 0007, 0226, 0231, 0004, 0135, 0055, 
  0110, 0014, 0341, 0112, 0316, 0050, 0356, 0230, 0207, 0227, 0310, 0045, 0267, 0162, 0067, 0010, 
  0114, 0055, 0343, 0036, 0077, 0070, 0317, 0146, 0156, 0064, 0257, 0337, 0072, 0377, 0214, 0146, 
  0342, 0226, 0230, 0202, 0212, 0241, 0370, 0365, 0137, 0221, 0373, 0062, 0270, 0300, 0136, 0371, 
  0015, 0202, 0136, 0140, 0011, 0000, 0050, 0165, 0165, 0141, 0171, 0051
} };
#else /* _MSC_VER */
static const SECTION union { const guint8 data[861]; const double alignment; void * const ptr;}  wayward_resource_data = {
  "\107\126\141\162\151\141\156\164\000\000\000\000\000\000\000\000"
  "\030\000\000\000\254\000\000\000\000\000\000\050\005\000\000\000"
  "\000\000\000\000\003\000\000\000\004\000\000\000\004\000\000\000"
  "\004\000\000\000\210\131\114\047\003\000\000\000\254\000\000\000"
  "\010\000\114\000\264\000\000\000\270\000\000\000\113\120\220\013"
  "\002\000\000\000\270\000\000\000\004\000\114\000\274\000\000\000"
  "\300\000\000\000\324\265\002\000\377\377\377\377\300\000\000\000"
  "\001\000\114\000\304\000\000\000\310\000\000\000\372\112\320\174"
  "\001\000\000\000\310\000\000\000\015\000\114\000\330\000\000\000"
  "\334\000\000\000\320\077\013\266\000\000\000\000\334\000\000\000"
  "\011\000\166\000\350\000\000\000\134\003\000\000\167\141\171\167"
  "\141\162\144\057\004\000\000\000\157\162\147\057\003\000\000\000"
  "\057\000\000\000\001\000\000\000\162\141\163\160\142\145\162\162"
  "\171\055\160\151\057\000\000\000\000\000\000\000\163\164\171\154"
  "\145\056\143\163\163\000\000\000\266\015\000\000\001\000\000\000"
  "\170\332\305\227\341\156\233\060\020\307\277\363\024\247\356\113"
  "\033\101\233\264\123\272\302\267\165\332\036\140\117\140\260\103"
  "\254\030\214\154\223\214\115\175\367\141\114\122\050\306\141\133"
  "\350\260\204\224\263\357\376\347\237\317\066\271\133\170\000\013"
  "\170\346\105\045\150\272\125\160\375\174\003\367\313\325\143\120"
  "\277\236\140\217\262\122\170\213\073\317\273\075\240\352\200\004"
  "\016\012\224\023\006\277\152\067\200\030\045\273\124\360\062\307"
  "\101\302\031\027\041\304\254\066\105\336\213\327\361\240\230\221"
  "\306\141\164\070\100\373\073\025\250\152\274\173\316\001\112\024"
  "\335\117\215\161\330\122\105\042\200\176\022\262\222\212\144\243"
  "\171\177\130\143\335\242\261\356\145\363\014\272\151\206\122\022"
  "\102\316\163\142\372\012\204\061\315\323\020\126\017\305\017\143"
  "\072\206\330\044\272\265\041\270\300\104\004\122\125\254\347\336"
  "\332\007\141\133\373\201\142\265\015\141\171\014\335\232\005\302"
  "\264\224\306\016\075\172\146\326\341\226\357\211\030\235\073\142"
  "\305\026\135\267\004\174\130\336\176\272\351\145\156\200\066\070"
  "\273\104\121\211\051\037\007\032\177\324\055\162\152\066\113\247"
  "\025\037\133\105\047\133\033\110\013\357\367\142\333\314\177\032"
  "\132\303\302\211\366\024\066\056\225\342\371\070\327\156\045\036"
  "\155\137\233\347\277\023\061\251\137\036\011\052\012\331\306\073"
  "\125\027\321\155\070\354\214\170\033\334\055\227\060\236\354\306"
  "\027\340\351\136\267\350\017\326\307\242\221\012\212\215\304\331"
  "\275\261\066\174\034\007\337\233\235\363\266\307\233\267\004\214"
  "\200\176\145\110\244\064\017\142\136\227\101\026\302\203\251\220"
  "\123\111\366\073\006\070\202\032\122\346\103\037\321\067\265\373"
  "\174\334\020\356\053\307\301\140\302\241\322\051\214\015\317\125"
  "\260\101\031\145\125\010\127\137\004\257\323\370\216\162\171\325"
  "\351\226\364\147\035\145\265\236\177\207\171\103\116\272\302\031"
  "\252\314\165\150\025\266\313\132\105\155\222\116\312\343\060\055"
  "\364\165\356\346\124\360\173\113\071\123\342\265\334\073\250\000"
  "\360\122\061\232\237\174\255\225\154\302\317\072\127\233\152\347"
  "\372\370\353\165\264\244\173\201\102\206\361\203\352\305\163\115"
  "\306\234\351\076\070\106\230\017\104\337\061\142\303\223\122\272"
  "\006\350\264\260\340\205\153\214\044\214\044\212\140\157\344\123"
  "\364\164\212\050\121\037\030\124\321\332\007\226\231\004\135\055"
  "\110\014\341\112\316\050\356\230\207\227\310\045\267\162\067\010"
  "\114\055\343\036\077\070\317\146\156\064\257\337\072\377\214\146"
  "\342\226\230\202\212\241\370\365\137\221\373\062\270\300\136\371"
  "\015\202\136\140\011\000\050\165\165\141\171\051" };
#endif /* !_MSC_VER */

static GStaticResource static_resource = { wayward_resource_data.data, sizeof (wayward_resource_data.data) - 1 /* nul terminator */, NULL, NULL, NULL };
extern GResource *wayward_get_resource (void);
GResource *wayward_get_resource (void)
{
  return g_static_resource_get_resource (&static_resource);
}
/*
  If G_HAS_CONSTRUCTORS is true then the compiler support *both* constructors and
  destructors, in a sane way, including e.g. on library unload. If not you're on
  your own.

  Some compilers need #pragma to handle this, which does not work with macros,
  so the way you need to use this is (for constructors):

  #ifdef G_DEFINE_CONSTRUCTOR_NEEDS_PRAGMA
  #pragma G_DEFINE_CONSTRUCTOR_PRAGMA_ARGS(my_constructor)
  #endif
  G_DEFINE_CONSTRUCTOR(my_constructor)
  static void my_constructor(void) {
   ...
  }

*/

#ifndef __GTK_DOC_IGNORE__

#if  __GNUC__ > 2 || (__GNUC__ == 2 && __GNUC_MINOR__ >= 7)

#define G_HAS_CONSTRUCTORS 1

#define G_DEFINE_CONSTRUCTOR(_func) static void __attribute__((constructor)) _func (void);
#define G_DEFINE_DESTRUCTOR(_func) static void __attribute__((destructor)) _func (void);

#elif defined (_MSC_VER) && (_MSC_VER >= 1500)
/* Visual studio 2008 and later has _Pragma */

#include <stdlib.h>

#define G_HAS_CONSTRUCTORS 1

/* We do some weird things to avoid the constructors being optimized
 * away on VS2015 if WholeProgramOptimization is enabled. First we
 * make a reference to the array from the wrapper to make sure its
 * references. Then we use a pragma to make sure the wrapper function
 * symbol is always included at the link stage. Also, the symbols
 * need to be extern (but not dllexport), even though they are not
 * really used from another object file.
 */

/* We need to account for differences between the mangling of symbols
 * for Win32 (x86) and x64 programs, as symbols on Win32 are prefixed
 * with an underscore but symbols on x64 are not.
 */
#ifdef _WIN64
#define G_MSVC_SYMBOL_PREFIX ""
#else
#define G_MSVC_SYMBOL_PREFIX "_"
#endif

#define G_DEFINE_CONSTRUCTOR(_func) G_MSVC_CTOR (_func, G_MSVC_SYMBOL_PREFIX)
#define G_DEFINE_DESTRUCTOR(_func) G_MSVC_DTOR (_func, G_MSVC_SYMBOL_PREFIX)

#define G_MSVC_CTOR(_func,_sym_prefix) \
  static void _func(void); \
  extern int (* _array ## _func)(void);              \
  int _func ## _wrapper(void) { _func(); g_slist_find (NULL,  _array ## _func); return 0; } \
  __pragma(comment(linker,"/include:" _sym_prefix # _func "_wrapper")) \
  __pragma(section(".CRT$XCU",read)) \
  __declspec(allocate(".CRT$XCU")) int (* _array ## _func)(void) = _func ## _wrapper;

#define G_MSVC_DTOR(_func,_sym_prefix) \
  static void _func(void); \
  extern int (* _array ## _func)(void);              \
  int _func ## _constructor(void) { atexit (_func); g_slist_find (NULL,  _array ## _func); return 0; } \
   __pragma(comment(linker,"/include:" _sym_prefix # _func "_constructor")) \
  __pragma(section(".CRT$XCU",read)) \
  __declspec(allocate(".CRT$XCU")) int (* _array ## _func)(void) = _func ## _constructor;

#elif defined (_MSC_VER)

#define G_HAS_CONSTRUCTORS 1

/* Pre Visual studio 2008 must use #pragma section */
#define G_DEFINE_CONSTRUCTOR_NEEDS_PRAGMA 1
#define G_DEFINE_DESTRUCTOR_NEEDS_PRAGMA 1

#define G_DEFINE_CONSTRUCTOR_PRAGMA_ARGS(_func) \
  section(".CRT$XCU",read)
#define G_DEFINE_CONSTRUCTOR(_func) \
  static void _func(void); \
  static int _func ## _wrapper(void) { _func(); return 0; } \
  __declspec(allocate(".CRT$XCU")) static int (*p)(void) = _func ## _wrapper;

#define G_DEFINE_DESTRUCTOR_PRAGMA_ARGS(_func) \
  section(".CRT$XCU",read)
#define G_DEFINE_DESTRUCTOR(_func) \
  static void _func(void); \
  static int _func ## _constructor(void) { atexit (_func); return 0; } \
  __declspec(allocate(".CRT$XCU")) static int (* _array ## _func)(void) = _func ## _constructor;

#elif defined(__SUNPRO_C)

/* This is not tested, but i believe it should work, based on:
 * http://opensource.apple.com/source/OpenSSL098/OpenSSL098-35/src/fips/fips_premain.c
 */

#define G_HAS_CONSTRUCTORS 1

#define G_DEFINE_CONSTRUCTOR_NEEDS_PRAGMA 1
#define G_DEFINE_DESTRUCTOR_NEEDS_PRAGMA 1

#define G_DEFINE_CONSTRUCTOR_PRAGMA_ARGS(_func) \
  init(_func)
#define G_DEFINE_CONSTRUCTOR(_func) \
  static void _func(void);

#define G_DEFINE_DESTRUCTOR_PRAGMA_ARGS(_func) \
  fini(_func)
#define G_DEFINE_DESTRUCTOR(_func) \
  static void _func(void);

#else

/* constructors not supported for this compiler */

#endif

#endif /* __GTK_DOC_IGNORE__ */

#ifdef G_HAS_CONSTRUCTORS

#ifdef G_DEFINE_CONSTRUCTOR_NEEDS_PRAGMA
#pragma G_DEFINE_CONSTRUCTOR_PRAGMA_ARGS(resource_constructor)
#endif
G_DEFINE_CONSTRUCTOR(resource_constructor)
#ifdef G_DEFINE_DESTRUCTOR_NEEDS_PRAGMA
#pragma G_DEFINE_DESTRUCTOR_PRAGMA_ARGS(resource_destructor)
#endif
G_DEFINE_DESTRUCTOR(resource_destructor)

#else
#warning "Constructor not supported on this compiler, linking in resources will not work"
#endif

static void resource_constructor (void)
{
  g_static_resource_init (&static_resource);
}

static void resource_destructor (void)
{
  g_static_resource_fini (&static_resource);
}
