using System.Runtime.InteropServices;

namespace libvpkedit.Format
{
    internal static unsafe partial class Extern
    {
        [DllImport("libvpkeditc")]
        public static extern void* vpkedit_grp_open([MarshalAs(UnmanagedType.LPStr)] string path);

        [DllImport("libvpkeditc")]
        public static extern void* vpkedit_grp_open_with_options([MarshalAs(UnmanagedType.LPStr)] string path, PackFileOptions options);
    }

    public class GRP : PackFile
    {
        private protected unsafe GRP(void* handle) : base(handle) {}

        public new static GRP? Open(string path)
        {
            unsafe
            {
                var handle = Extern.vpkedit_grp_open(path);
                return handle == null ? null : new GRP(handle);
            }
        }

        public new static GRP? Open(string path, PackFileOptions options)
        {
            unsafe
            {
                var handle = Extern.vpkedit_grp_open_with_options(path, options);
                return handle == null ? null : new GRP(handle);
            }
        }
    }
}
