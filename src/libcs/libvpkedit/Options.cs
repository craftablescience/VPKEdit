using System.Runtime.InteropServices;

namespace libvpkedit
{
    [StructLayout(LayoutKind.Sequential)]
    public struct PackFileOptions
    {
        byte gma_writeCRCs;

        uint vpk_version;

        uint vpk_preferredChunkSize;

        byte vpk_generateMD5Entries;

        ushort zip_compressionMethod;
    }
}
