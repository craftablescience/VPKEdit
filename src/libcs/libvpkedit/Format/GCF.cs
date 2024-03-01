using System.Runtime.InteropServices;

namespace libvpkedit.Format
{
    internal unsafe static partial class Extern
    {
        [DllImport("libvpkeditc")]
        public static extern void* vpkedit_gcf_open([MarshalAs(UnmanagedType.LPStr)] string path);

        [DllImport("libvpkeditc")]
        public static extern void* vpkedit_gcf_open_with_options([MarshalAs(UnmanagedType.LPStr)] string path, PackFileOptions options);
    }

    public class GCF : PackFile
    {
        private protected unsafe GCF(void* handle) : base(handle) {}

        public new static GCF? Open(string path)
        {
            unsafe
            {
                void* handle = Extern.vpkedit_gcf_open(path);
                if (handle == null)
                {
                    return null;
                }
                return new GCF(handle);
            }
        }

        public new static GCF? Open(string path, PackFileOptions options)
        {
            unsafe
            {
                void* handle = Extern.vpkedit_gcf_open_with_options(path, options);
                if (handle == null)
                {
                    return null;
                }
                return new GCF(handle);
            }
        }
    }
}
