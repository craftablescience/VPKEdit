using System;
using System.Runtime.InteropServices;

namespace libvpkedit.Format
{
    internal static unsafe partial class Extern
    {
        [DllImport("libvpkeditc")]
        public static extern void* vpkedit_vpk_create_empty([MarshalAs(UnmanagedType.LPStr)] string path);

        [DllImport("libvpkeditc")]
        public static extern void* vpkedit_vpk_create_empty_with_options([MarshalAs(UnmanagedType.LPStr)] string path, PackFileOptions options);

        [DllImport("libvpkeditc")]
        public static extern void* vpkedit_vpk_create_from_directory([MarshalAs(UnmanagedType.LPStr)] string vpkPath, [MarshalAs(UnmanagedType.LPStr)] string contentPath, byte saveToDir);

        [DllImport("libvpkeditc")]
        public static extern void* vpkedit_vpk_create_from_directory_with_options([MarshalAs(UnmanagedType.LPStr)] string vpkPath, [MarshalAs(UnmanagedType.LPStr)] string contentPath, byte saveToDir, PackFileOptions options);

        [DllImport("libvpkeditc")]
        public static extern void* vpkedit_vpk_open([MarshalAs(UnmanagedType.LPStr)] string path);

        [DllImport("libvpkeditc")]
        public static extern void* vpkedit_vpk_open_with_options([MarshalAs(UnmanagedType.LPStr)] string path, PackFileOptions options);

        [DllImport("libvpkeditc")]
        public static extern uint vpkedit_vpk_get_version(void* handle);

        [DllImport("libvpkeditc")]
        public static extern void vpkedit_vpk_set_version(void* handle, uint version);
    }

    public class VPK : PackFile
    {
        private unsafe VPK(void* handle) : base(handle) {}

        public static VPK? CreateEmpty(string path)
        {
            unsafe
            {
                var handle = Extern.vpkedit_vpk_create_empty(path);
                return handle == null ? null : new VPK(handle);
            }
        }

        public static VPK? CreateEmpty(string path, PackFileOptions options)
        {
            unsafe
            {
                var handle = Extern.vpkedit_vpk_create_empty_with_options(path, options);
                return handle == null ? null : new VPK(handle);
            }
        }

        public static VPK? CreateFromDirectory(string vpkPath, string contentPath, bool saveToDir = false)
        {
            unsafe
            {
                var handle = Extern.vpkedit_vpk_create_from_directory(vpkPath, contentPath, Convert.ToByte(saveToDir));
                return handle == null ? null : new VPK(handle);
            }
        }

        public static VPK? CreateFromDirectory(string vpkPath, string contentPath, PackFileOptions options, bool saveToDir = false)
        {
            unsafe
            {
                var handle = Extern.vpkedit_vpk_create_from_directory_with_options(vpkPath, contentPath, Convert.ToByte(saveToDir), options);
                return handle == null ? null : new VPK(handle);
            }
        }

        public new static VPK? Open(string path)
        {
            unsafe
            {
                var handle = Extern.vpkedit_vpk_open(path);
                return handle == null ? null : new VPK(handle);
            }
        }

        public new static VPK? Open(string path, PackFileOptions options)
        {
            unsafe
            {
                var handle = Extern.vpkedit_vpk_open_with_options(path, options);
                return handle == null ? null : new VPK(handle);
            }
        }

        public uint Version
        {
            get
            {
                unsafe
                {
                    return Extern.vpkedit_vpk_get_version(Handle);
                }
            }
            set
            {
                unsafe
                {
                    Extern.vpkedit_vpk_set_version(Handle, Math.Clamp(value, 1, 2));
                }
            }
        }
    }
}
