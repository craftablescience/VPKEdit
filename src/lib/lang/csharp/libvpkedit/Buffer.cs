using System;
using System.Runtime.InteropServices;

namespace libvpkedit
{
    internal static unsafe partial class Extern
    {
        [DllImport("libvpkeditc")]
        public static extern Buffer vpkedit_buffer_new(ulong size);

        [DllImport("libvpkeditc")]
        public static extern void vpkedit_buffer_free(Buffer* str);
    }

    [StructLayout(LayoutKind.Sequential)]
    internal struct Buffer
    {
        public long size;
        public unsafe byte* data;
    }

    internal static unsafe class BufferUtils
    {
        public static byte[] ConvertToArrayAndDelete(ref Buffer buffer)
        {
            var result = new Span<byte>(buffer.data, (int) buffer.size).ToArray();

            fixed (Buffer* bufferPtr = &buffer)
            {
                Extern.vpkedit_buffer_free(bufferPtr);
            }

            return result;
        }
    }
}
