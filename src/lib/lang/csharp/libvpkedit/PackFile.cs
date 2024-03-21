using System;
using System.Collections.Generic;
using System.Linq;
using System.Runtime.InteropServices;

namespace libvpkedit
{
    internal static unsafe partial class Extern
    {
        [DllImport("libvpkeditc")]
        public static extern void* vpkedit_open([MarshalAs(UnmanagedType.LPStr)] string path);

        [DllImport("libvpkeditc")]
        public static extern void* vpkedit_open_with_options([MarshalAs(UnmanagedType.LPStr)] string path, PackFileOptions options);

        [DllImport("libvpkeditc")]
        public static extern PackFileType vpkedit_get_type(void* handle);

        [DllImport("libvpkeditc")]
        public static extern PackFileOptions vpkedit_get_options(void* handle);

        [DllImport("libvpkeditc")]
        public static extern StringArray vpkedit_verify_entry_checksums(void* handle);

        [DllImport("libvpkeditc")]
        public static extern byte vpkedit_verify_file_checksum(void* handle);

        [DllImport("libvpkeditc")]
        public static extern byte vpkedit_is_case_sensitive(void* handle);

        [DllImport("libvpkeditc")]
        public static extern void* vpkedit_find_entry(void* handle, [MarshalAs(UnmanagedType.LPStr)] string filename, byte includeUnbaked);

        [DllImport("libvpkeditc")]
        public static extern Buffer vpkedit_read_entry(void* handle, void* entry);

        [DllImport("libvpkeditc")]
        public static extern String vpkedit_read_entry_text(void* handle, void* entry);

        [DllImport("libvpkeditc")]
        public static extern byte vpkedit_is_read_only(void* handle);

        [DllImport("libvpkeditc")]
        public static extern void vpkedit_add_entry_from_file(void* handle, [MarshalAs(UnmanagedType.LPStr)] string filename, [MarshalAs(UnmanagedType.LPStr)] string pathToFile);

        [DllImport("libvpkeditc")]
        public static extern void vpkedit_add_entry_from_mem(void* handle, [MarshalAs(UnmanagedType.LPStr)] string filename, byte* buffer, ulong bufferLen);

        [DllImport("libvpkeditc")]
        public static extern byte vpkedit_remove_entry(void* handle, [MarshalAs(UnmanagedType.LPStr)] string filename);

        [DllImport("libvpkeditc")]
        public static extern byte vpkedit_bake(void* handle, [MarshalAs(UnmanagedType.LPStr)] string outputDir);

        [DllImport("libvpkeditc")]
        public static extern EntryHandleArray vpkedit_get_baked_entries(void* handle);

        [DllImport("libvpkeditc")]
        public static extern EntryHandleArray vpkedit_get_unbaked_entries(void* handle);

        [DllImport("libvpkeditc")]
        public static extern ulong vpkedit_get_entry_count(void* handle, byte includeUnbaked);

        [DllImport("libvpkeditc")]
        public static extern Buffer vpkedit_read_virtual_entry(void* handle, void* entry);

        [DllImport("libvpkeditc")]
        public static extern byte vpkedit_overwrite_virtual_entry_from_file(void* handle, void* entry, [MarshalAs(UnmanagedType.LPStr)] string pathToFile);

        [DllImport("libvpkeditc")]
        public static extern byte vpkedit_overwrite_virtual_entry_from_mem(void* handle, void* entry, byte* buffer, ulong bufferLen);

        [DllImport("libvpkeditc")]
        public static extern VirtualEntryHandleArray vpkedit_get_virtual_entries(void* handle);
        
        [DllImport("libvpkeditc")]
        public static extern ulong vpkedit_get_filepath(void* handle, sbyte* buffer, ulong bufferLen);

        [DllImport("libvpkeditc")]
        public static extern ulong vpkedit_get_truncated_filepath(void* handle, sbyte* buffer, ulong bufferLen);

        [DllImport("libvpkeditc")]
        public static extern ulong vpkedit_get_filename(void* handle, sbyte* buffer, ulong bufferLen);

        [DllImport("libvpkeditc")]
        public static extern ulong vpkedit_get_truncated_filename(void* handle, sbyte* buffer, ulong bufferLen);

        [DllImport("libvpkeditc")]
        public static extern ulong vpkedit_get_filestem(void* handle, sbyte* buffer, ulong bufferLen);

        [DllImport("libvpkeditc")]
        public static extern ulong vpkedit_get_truncated_filestem(void* handle, sbyte* buffer, ulong bufferLen);

        [DllImport("libvpkeditc")]
        public static extern ulong vpkedit_get_supported_entry_attributes(void* handle, Attribute* buffer, ulong bufferLen);

        [DllImport("libvpkeditc")]
        public static extern ulong vpkedit_to_string(void* handle, sbyte* buffer, ulong bufferLen);

        [DllImport("libvpkeditc")]
        public static extern void vpkedit_close(void** handle);

        [DllImport("libvpkeditc")]
        public static extern StringArray vpkedit_get_supported_file_types();
    }

    public class PackFile
    {
        private protected unsafe PackFile(void* handle)
        {
            Handle = handle;
        }

        ~PackFile()
        {
            unsafe
            {
                fixed (void** handlePtr = &Handle)
                {
                    Extern.vpkedit_close(handlePtr);
                }
            }
        }

        public static PackFile? Open(string path)
        {
            unsafe
            {
                var handle = Extern.vpkedit_open(path);
                return handle == null ? null : new PackFile(handle);
            }
        }

        public static PackFile? Open(string path, PackFileOptions options)
        {
            unsafe
            {
                var handle = Extern.vpkedit_open_with_options(path, options);
                return handle == null ? null : new PackFile(handle);
            }
        }

        public static List<string> GetSupportedFileTypes()
        {
            var stringArray = Extern.vpkedit_get_supported_file_types();
            return StringUtils.ConvertToListAndDelete(ref stringArray);
        }

        public IEnumerable<string> VerifyEntryChecksums()
        {
            unsafe
            {
                var stringArray = Extern.vpkedit_verify_entry_checksums(Handle);
                return StringUtils.ConvertToListAndDelete(ref stringArray);
            }
        }

        public bool VerifyFileChecksum()
        {
            unsafe
            {
                return Convert.ToBoolean(Extern.vpkedit_verify_file_checksum(Handle));
            }
        }

        public Entry? FindEntry(string filename, bool includeUnbaked = true)
        {
            unsafe
            {
                var entry = Extern.vpkedit_find_entry(Handle, filename, Convert.ToByte(includeUnbaked));
                return entry == null ? null : new Entry(entry, false);
            }
        }

        public byte[]? ReadEntry(Entry entry)
        {
            unsafe
            {
                var buffer = Extern.vpkedit_read_entry(Handle, entry.Handle);
                return buffer.size < 0 ? null : BufferUtils.ConvertToArrayAndDelete(ref buffer);
            }
        }

        public string? ReadEntryText(Entry entry)
        {
            unsafe
            {
                var str = Extern.vpkedit_read_entry_text(Handle, entry.Handle);
                return str.size < 0 ? null : StringUtils.ConvertToStringAndDelete(ref str);
            }
        }

        public void AddEntry(string filename, string pathToFile)
        {
            unsafe
            {
                Extern.vpkedit_add_entry_from_file(Handle, filename, pathToFile);
            }
        }

        public void AddEntry(string filename, byte[] buffer)
        {
            unsafe
            {
                fixed (byte* bufferPtr = buffer)
                {
                    Extern.vpkedit_add_entry_from_mem(Handle, filename, bufferPtr, (ulong) buffer.LongLength);
                }
            }
        }

        public void AddEntry(string filename, IEnumerable<byte> buffer)
        {
            unsafe
            {
                var data = buffer.ToArray();
                fixed (byte* bufferPtr = data)
                {
                    Extern.vpkedit_add_entry_from_mem(Handle, filename, bufferPtr, (ulong) data.LongLength);
                }
            }
        }

        public bool RemoveEntry(string filename)
        {
            unsafe
            {
                return Convert.ToBoolean(Extern.vpkedit_remove_entry(Handle, filename));
            }
        }

        public bool Bake(string outputDir)
        {
            unsafe
            {
                return Convert.ToBoolean(Extern.vpkedit_bake(Handle, outputDir));
            }
        }

        public EntryEnumerable GetBakedEntries()
        {
            unsafe
            {
                return new EntryEnumerable(Extern.vpkedit_get_baked_entries(Handle));
            }
        }

        public EntryEnumerable GetUnbakedEntries()
        {
            unsafe
            {
                return new EntryEnumerable(Extern.vpkedit_get_unbaked_entries(Handle));
            }
        }

        public ulong GetEntryCount(bool includeUnbaked = true)
        {
            unsafe
            {
                return Extern.vpkedit_get_entry_count(Handle, Convert.ToByte(includeUnbaked));
            }
        }

        public byte[]? ReadVirtualEntry(VirtualEntry entry)
        {
            unsafe
            {
                var buffer = Extern.vpkedit_read_virtual_entry(Handle, entry.Handle);
                return buffer.size < 0 ? null : BufferUtils.ConvertToArrayAndDelete(ref buffer);
            }
        }

        public void OverwriteVirtualEntry(VirtualEntry entry, string pathToFile)
        {
            unsafe
            {
                Extern.vpkedit_overwrite_virtual_entry_from_file(Handle, entry.Handle, pathToFile);
            }
        }

        public bool OverwriteVirtualEntry(VirtualEntry entry, byte[] buffer)
        {
            unsafe
            {
                fixed (byte* bufferPtr = buffer)
                {
                    return Convert.ToBoolean(Extern.vpkedit_overwrite_virtual_entry_from_mem(Handle, entry.Handle, bufferPtr, (ulong) buffer.LongLength));
                }
            }
        }

        public bool OverwriteVirtualEntry(VirtualEntry entry, IEnumerable<byte> buffer)
        {
            unsafe
            {
                var data = buffer.ToArray();
                fixed (byte* dataPtr = data)
                {
                    return Convert.ToBoolean(Extern.vpkedit_overwrite_virtual_entry_from_mem(Handle, entry.Handle, dataPtr, (ulong) data.LongLength));
                }
            }
        }

        public VirtualEntryEnumerable GetVirtualEntries()
        {
            unsafe
            {
                return new VirtualEntryEnumerable(Extern.vpkedit_get_virtual_entries(Handle));
            }
        }

        public override string ToString()
        {
            unsafe
            {
                var stringPtr = stackalloc sbyte[Constants.MaxPackFileString];
                Extern.vpkedit_to_string(Handle, stringPtr, Convert.ToUInt64(Constants.MaxPackFileString));
                return new string(stringPtr);
            }
        }

        public PackFileType Type
        {
            get
            {
                unsafe
                {
                    return Extern.vpkedit_get_type(Handle);
                }
            }
        }

        public PackFileOptions Options
        {
            get
            {
                unsafe
                {
                    return Extern.vpkedit_get_options(Handle);
                }
            }
        }

        public bool CaseSensitive
        {
            get
            {
                unsafe
                {
                    return Convert.ToBoolean(Extern.vpkedit_is_case_sensitive(Handle));
                }
            }
        }

        public bool ReadOnly
        {
            get
            {
                unsafe
                {
                    return Convert.ToBoolean(Extern.vpkedit_is_read_only(Handle));
                }
            }
        }

        public string FilePath
        {
            get
            {
                Span<sbyte> stringArray = new sbyte[Constants.MaxPath];
                unsafe
                {
                    fixed (sbyte* stringPtr = stringArray)
                    {
                        Extern.vpkedit_get_filepath(Handle, stringPtr, Convert.ToUInt64(stringArray.Length));
                        return new string(stringPtr);
                    }
                }
            }
        }

        public string TruncatedFilePath
        {
            get
            {
                Span<sbyte> stringArray = new sbyte[Constants.MaxPath];
                unsafe
                {
                    fixed (sbyte* stringPtr = stringArray)
                    {
                        Extern.vpkedit_get_truncated_filepath(Handle, stringPtr, Convert.ToUInt64(stringArray.Length));
                        return new string(stringPtr);
                    }
                }
            }
        }

        public string FileName
        {
            get
            {
                Span<sbyte> stringArray = new sbyte[Constants.MaxFilename];
                unsafe
                {
                    fixed (sbyte* stringPtr = stringArray)
                    {
                        Extern.vpkedit_get_filename(Handle, stringPtr, Convert.ToUInt64(stringArray.Length));
                        return new string(stringPtr);
                    }
                }
            }
        }

        public string TruncatedFileName
        {
            get
            {
                Span<sbyte> stringArray = new sbyte[Constants.MaxFilename];
                unsafe
                {
                    fixed (sbyte* stringPtr = stringArray)
                    {
                        Extern.vpkedit_get_truncated_filename(Handle, stringPtr, Convert.ToUInt64(stringArray.Length));
                        return new string(stringPtr);
                    }
                }
            }
        }

        public string FileStem
        {
            get
            {
                Span<sbyte> stringArray = new sbyte[Constants.MaxFilename];
                unsafe
                {
                    fixed (sbyte* stringPtr = stringArray)
                    {
                        Extern.vpkedit_get_filestem(Handle, stringPtr, Convert.ToUInt64(stringArray.Length));
                        return new string(stringPtr);
                    }
                }
            }
        }

        public string TruncatedFileStem
        {
            get
            {
                Span<sbyte> stringArray = new sbyte[Constants.MaxFilename];
                unsafe
                {
                    fixed (sbyte* stringPtr = stringArray)
                    {
                        Extern.vpkedit_get_truncated_filestem(Handle, stringPtr, Convert.ToUInt64(stringArray.Length));
                        return new string(stringPtr);
                    }
                }
            }
        }

        public List<Attribute> SupportedEntryAttributes
        {
            get
            {
                unsafe
                {
                    var attrArray = stackalloc Attribute[(int) Attribute.COUNT];
                    var numAttributes = Extern.vpkedit_get_supported_entry_attributes(Handle, attrArray, (ulong) Attribute.COUNT);

                    var result = new List<Attribute>();
                    for (ulong i = 0; i < numAttributes; i++)
                    {
                        result.Add(attrArray[i]);
                    }
                    return result;
                }
            }
        }

        private protected readonly unsafe void* Handle;
    }
}
