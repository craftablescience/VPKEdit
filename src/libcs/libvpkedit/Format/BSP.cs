using System.Runtime.InteropServices;

namespace libvpkedit.Format
{
    internal unsafe static partial class Extern
    {
        [DllImport("libvpkeditc.dll")]
        public static extern void* vpkedit_bsp_open([MarshalAs(UnmanagedType.LPStr)] string path);

        [DllImport("libvpkeditc.dll")]
        public static extern void* vpkedit_bsp_open_with_options([MarshalAs(UnmanagedType.LPStr)] string path, PackFileOptions options);
    }

    public class BSP : PackFile
    {
        private protected unsafe BSP(void* handle) : base(handle) {}

        public new static BSP? Open(string path)
        {
            unsafe
            {
                void* handle = Extern.vpkedit_bsp_open(path);
                if (handle == null)
                {
                    return null;
                }
                return new BSP(handle);
            }
        }

        public new static BSP? Open(string path, PackFileOptions options)
        {
            unsafe
            {
                void* handle = Extern.vpkedit_bsp_open_with_options(path, options);
                if (handle == null)
                {
                    return null;
                }
                return new BSP(handle);
            }
        }
    }
}
