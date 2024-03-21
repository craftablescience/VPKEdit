using System.Runtime.InteropServices;

namespace libvpkedit.Format
{
    internal static unsafe partial class Extern
    {
        [DllImport("libvpkeditc")]
        public static extern void* vpkedit_pck_open([MarshalAs(UnmanagedType.LPStr)] string path);

        [DllImport("libvpkeditc")]
        public static extern void* vpkedit_pck_open_with_options([MarshalAs(UnmanagedType.LPStr)] string path, PackFileOptions options);
    }

    public class PCK : PackFile
    {
        private protected unsafe PCK(void* handle) : base(handle) {}

        public new static PCK? Open(string path)
        {
            unsafe
            {
                var handle = Extern.vpkedit_pck_open(path);
                return handle == null ? null : new PCK(handle);
            }
        }

        public new static PCK? Open(string path, PackFileOptions options)
        {
            unsafe
            {
                var handle = Extern.vpkedit_pck_open_with_options(path, options);
                return handle == null ? null : new PCK(handle);
            }
        }
    }
}
