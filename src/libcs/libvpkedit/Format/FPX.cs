using System.Runtime.InteropServices;

namespace libvpkedit.Format
{
    internal unsafe static partial class Extern
    {
        [DllImport("libvpkeditc.dll")]
        public static extern void* vpkedit_fpx_open([MarshalAs(UnmanagedType.LPStr)] string path);

        [DllImport("libvpkeditc.dll")]
        public static extern void* vpkedit_fpx_open_with_options([MarshalAs(UnmanagedType.LPStr)] string path, PackFileOptions options);
    }

    public class FPX : PackFile
    {
        private protected unsafe FPX(void* handle) : base(handle) {}

        public new static FPX? Open(string path)
        {
            unsafe
            {
                void* handle = Extern.vpkedit_fpx_open(path);
                if (handle == null)
                {
                    return null;
                }
                return new FPX(handle);
            }
        }

        public new static FPX? Open(string path, PackFileOptions options)
        {
            unsafe
            {
                void* handle = Extern.vpkedit_fpx_open_with_options(path, options);
                if (handle == null)
                {
                    return null;
                }
                return new FPX(handle);
            }
        }
    }
}
