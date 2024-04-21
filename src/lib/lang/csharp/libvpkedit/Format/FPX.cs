using System.Runtime.InteropServices;

namespace libvpkedit.Format
{
    internal static unsafe partial class Extern
    {
        [DllImport("libvpkeditc")]
        public static extern void* vpkedit_fpx_open([MarshalAs(UnmanagedType.LPStr)] string path);

        [DllImport("libvpkeditc")]
        public static extern void* vpkedit_fpx_open_with_options([MarshalAs(UnmanagedType.LPStr)] string path, PackFileOptions options);
    }

    public class FPX : PackFile
    {
        private protected unsafe FPX(void* handle) : base(handle) {}

        public new static FPX? Open(string path)
        {
            unsafe
            {
                var handle = Extern.vpkedit_fpx_open(path);
                return handle == null ? null : new FPX(handle);
            }
        }

        public new static FPX? Open(string path, PackFileOptions options)
        {
            unsafe
            {
                var handle = Extern.vpkedit_fpx_open_with_options(path, options);
                return handle == null ? null : new FPX(handle);
            }
        }
    }
}
