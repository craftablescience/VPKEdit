using System.Runtime.InteropServices;

namespace libvpkedit.Format
{
    internal unsafe static partial class Extern
    {
        [DllImport("libvpkeditc.dll")]
        public static extern void* vpkedit_pak_open([MarshalAs(UnmanagedType.LPStr)] string path);

        [DllImport("libvpkeditc.dll")]
        public static extern void* vpkedit_pak_open_with_options([MarshalAs(UnmanagedType.LPStr)] string path, PackFileOptions options);
    }

    public class PAK : PackFile
    {
        private protected unsafe PAK(void* handle) : base(handle) {}

        public new static PAK? Open(string path)
        {
            unsafe
            {
                void* handle = Extern.vpkedit_pak_open(path);
                if (handle == null)
                {
                    return null;
                }
                return new PAK(handle);
            }
        }

        public new static PAK? Open(string path, PackFileOptions options)
        {
            unsafe
            {
                void* handle = Extern.vpkedit_pak_open_with_options(path, options);
                if (handle == null)
                {
                    return null;
                }
                return new PAK(handle);
            }
        }
    }
}
