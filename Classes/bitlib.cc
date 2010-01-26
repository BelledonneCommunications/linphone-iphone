#include "bitlib.hh"
#include <stdlib.h>
typedef unsigned int UInt32;
namespace BitLib
{

        // ----------------------------------------------------------------------------------------
        UInt64 Bit::Add(int start, int length, UInt64 val, UInt32 numToAdd)
        {
            int end = start + length - 1;

            UInt64 result = val;

            // Clear out bits that will be modified
            for (int i = start; i <= end; i++)
            {
                result &= ~((UInt64)1 << i);
            }

            UInt64 n = val >> start;
            n += numToAdd;

            for (int i = 0; i < length; i++)
            {
                UInt64 b = n & ((UInt64)1 << i);
                b = b << start;
                result |= b;
            }

            return result;
        }

        // ----------------------------------------------------------------------------------------
        UInt64 Bit::Subtract(int start, int length, UInt64 val, UInt32 numToSubtract)
        {
            int end = start + length - 1;

            UInt64 result = val;

            // Clear out bits that will be modified
            for (int i = start; i <= end; i++)
            {
                result &= ~((UInt64)1 << i);
            }

            UInt64 n = val >> start;
            n -= numToSubtract;

            for (int i = 0; i < length; i++)
            {
                UInt64 b = n & ((UInt64)1 << i);
                b = b << start;
                result |= b;
            }

            return result;
        }

        // ----------------------------------------------------------------------------------------
        UInt64 Bit::Xor(int start, int length, UInt64 val, UInt32 val2)
        {
            int end = start + length - 1;

            UInt64 result = val;

            for (int i = 0; i < length; i++)
            {
                UInt64 b = val2 & ((UInt64)1 << i);
                b = b << start;
                result ^= b;
            }

            return result;
        }

        // ----------------------------------------------------------------------------------------
        UInt64 Bit::RotateLeft(int start, int length, UInt64 val)
        {
            if (length <= 1)
            {
                return val;
            }

            int end = start + length - 1;

            if (end >= 48)
            {
                 abort();
            }

            UInt64 result = val;

            // Clear out bits that will be modified
            for (int i = start; i <= end; i++)
            {
                result &=  ~((UInt64)1 << i);
            }

            // Shift all bits except the last
            for (int i = start; i < end; i++)
            {
                UInt64 b = val & ((UInt64)1 << i);
                result |= (b << 1);
            }

            // Wrap the last bit
            {
                UInt64 b = val & ((UInt64)1 << end);
                result |= (b >> (length-1));
            }

            return result;
        }

        // ----------------------------------------------------------------------------------------
        UInt64 Bit::RotateRight(int start, int length, UInt64 val)
        {
            if (length <= 1)
            {
                return val;
            }

            int end = start + length - 1;

            if (end >= 48)
            {
                abort();
            }

            UInt64 result = val;

            // Clear out bits that will be modified
            for (int i = start; i <= end; i++)
            {
                result &= ~((UInt64)1 << i);
            }

            // Shift all bits except the first
            for (int i = start+1; i <= end; i++)
            {
                UInt64 b = val & ((UInt64)1 << i);
                result |= (b >> 1);
            }

            // Wrap the first bit
            {
                UInt64 b = val & ((UInt64)1 << start);
                result |= (b << (length - 1));
            }

            return result;
        }

        // ----------------------------------------------------------------------------------------
        UInt64 Bit::Swizzle(UInt64 val, const int operations[], int op_len)
        {
            int index = 0;

            while (index < op_len)
            {
                Operation op = (Operation)operations[index++];

                switch (op)
                {
                    case OpRor:
                        {
                            int start = operations[index++];
                            int length = operations[index++];
                            val = RotateRight(start, length, val);
                            break;
                        }

                    case OpRol:
                        {
                            int start = operations[index++];
                            int length = operations[index++];
                            val = RotateLeft(start, length, val);
                            break;
                        }

                    case OpAdd:
                        {
                            int start = operations[index++];
                            int length = operations[index++];
                            UInt32 toAdd = (UInt32)operations[index++];
                            val = Add(start, length, val, toAdd);
                            break;
                        }

                    case OpSubtract:
                        {
                            int start = operations[index++];
                            int length = operations[index++];
                            UInt32 toSubtract = (UInt32)operations[index++];
                            val = Subtract(start, length, val, toSubtract);
                            break;
                        }

                    case OpXor:
                        {
                            int start = operations[index++];
                            int length = operations[index++];
                            UInt32 toXor = (UInt32)operations[index++];
                            val = Xor(start, length, val, toXor);
                            break;
                        }

                    default:
                        break;
                }
            }

            return val;
        }

}
