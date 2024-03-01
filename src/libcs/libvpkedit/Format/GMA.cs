using System.Runtime.InteropServices;

namespace libvpkedit.Format
{
    internal unsafe static partial class Extern
    {
        [DllImport("libvpkeditc.dll")]
        public static extern void* vpkedit_gma_open([MarshalAs(UnmanagedType.LPStr)] string path);

        [DllImport("libvpkeditc.dll")]
        public static extern void* vpkedit_gma_open_with_options([MarshalAs(UnmanagedType.LPStr)] string path, PackFileOptions options);
    }

    public class GMA : PackFile
    {
        private protected unsafe GMA(void* handle) : base(handle) {}

        public new static GMA? Open(string path)
        {
            unsafe
            {
                void* handle = Extern.vpkedit_gma_open(path);
                if (handle == null)
                {
                    return null;
                }
                return new GMA(handle);
            }
        }

        public new static GMA? Open(string path, PackFileOptions options)
        {
            unsafe
            {
                void* handle = Extern.vpkedit_gma_open_with_options(path, options);
                if (handle == null)
                {
                    return null;
                }
                return new GMA(handle);
            }
        }
    }
}
