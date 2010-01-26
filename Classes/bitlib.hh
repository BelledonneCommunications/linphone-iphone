#include <stdint.h>
#include <stdlib.h>

typedef uint64_t UInt64;
typedef unsigned int UInt32;
typedef uint8_t byte;
typedef unsigned int uint;
namespace BitLib{
	class Bit{
		public:
			enum Operation{
			    OpRor = 0,
			    OpRol,
			    OpAdd,
			    OpSubtract,
			    OpXor,
			    OpMax
			};
			static UInt64 Add(int start, int length, UInt64 val, UInt32 numToAdd);
			static UInt64 Subtract(int start, int length, UInt64 val, UInt32 numToSubtract);
			static UInt64 Xor(int start, int length, UInt64 val, UInt32 val2);
			static UInt64 RotateLeft(int start, int length, UInt64 val);
			static UInt64 RotateRight(int start, int length, UInt64 val);
			static UInt64 Swizzle(UInt64 val, const int operations[], int oplen);
	};
}
