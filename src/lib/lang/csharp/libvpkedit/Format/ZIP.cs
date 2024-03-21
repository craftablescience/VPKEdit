using System.Runtime.InteropServices;

namespace libvpkedit.Format
{
    internal static unsafe partial class Extern
    {
        [DllImport("libvpkeditc")]
        public static extern void* vpkedit_zip_open([MarshalAs(UnmanagedType.LPStr)] string path);

        [DllImport("libvpkeditc")]
        public static extern void* vpkedit_zip_open_with_options([MarshalAs(UnmanagedType.LPStr)] string path, PackFileOptions options);
    }

    public class ZIP : PackFile
    {
        private protected unsafe ZIP(void* handle) : base(handle) {}

        public new static ZIP? Open(string path)
        {
            unsafe
            {
                var handle = Extern.vpkedit_zip_open(path);
                return handle == null ? null : new ZIP(handle);
            }
        }

        public new static ZIP? Open(string path, PackFileOptions options)
        {
            unsafe
            {
                var handle = Extern.vpkedit_zip_open_with_options(path, options);
                return handle == null ? null : new ZIP(handle);
            }
        }
    }
}
