using System.Runtime.InteropServices;

namespace libvpkedit.Format
{
    internal unsafe static partial class Extern
    {
        [DllImport("libvpkeditc.dll")]
        public static extern void* vpkedit_grp_open([MarshalAs(UnmanagedType.LPStr)] string path);

        [DllImport("libvpkeditc.dll")]
        public static extern void* vpkedit_grp_open_with_options([MarshalAs(UnmanagedType.LPStr)] string path, PackFileOptions options);
    }

    public class GRP : PackFile
    {
        private protected unsafe GRP(void* handle) : base(handle) {}

        public new static GRP? Open(string path)
        {
            unsafe
            {
                void* handle = Extern.vpkedit_grp_open(path);
                if (handle == null)
                {
                    return null;
                }
                return new GRP(handle);
            }
        }

        public new static GRP? Open(string path, PackFileOptions options)
        {
            unsafe
            {
                void* handle = Extern.vpkedit_grp_open_with_options(path, options);
                if (handle == null)
                {
                    return null;
                }
                return new GRP(handle);
            }
        }
    }
}
