using System.Runtime.InteropServices;

namespace libvpkedit.Format
{
    internal unsafe static partial class Extern
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
                void* handle = Extern.vpkedit_zip_open(path);
                if (handle == null)
                {
                    return null;
                }
                return new ZIP(handle);
            }
        }

        public new static ZIP? Open(string path, PackFileOptions options)
        {
            unsafe
            {
                void* handle = Extern.vpkedit_zip_open_with_options(path, options);
                if (handle == null)
                {
                    return null;
                }
                return new ZIP(handle);
            }
        }
    }
}
