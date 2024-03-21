using System.Runtime.InteropServices;

namespace libvpkedit.Format
{
    internal static unsafe partial class Extern
    {
        [DllImport("libvpkeditc")]
        public static extern void* vpkedit_pak_open([MarshalAs(UnmanagedType.LPStr)] string path);

        [DllImport("libvpkeditc")]
        public static extern void* vpkedit_pak_open_with_options([MarshalAs(UnmanagedType.LPStr)] string path, PackFileOptions options);
    }

    public class PAK : PackFile
    {
        private protected unsafe PAK(void* handle) : base(handle) {}

        public new static PAK? Open(string path)
        {
            unsafe
            {
                var handle = Extern.vpkedit_pak_open(path);
                return handle == null ? null : new PAK(handle);
            }
        }

        public new static PAK? Open(string path, PackFileOptions options)
        {
            unsafe
            {
                var handle = Extern.vpkedit_pak_open_with_options(path, options);
                return handle == null ? null : new PAK(handle);
            }
        }
    }
}
