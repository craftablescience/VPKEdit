using System;
using System.Collections.Generic;
using System.Linq;
using System.Runtime.InteropServices;

namespace libvpkedit
{
    internal unsafe static partial class Extern
    {
        [DllImport("libvpkeditc.dll")]
        public static extern void* vpkedit_open([MarshalAs(UnmanagedType.LPStr)] string path);

        [DllImport("libvpkeditc.dll")]
        public static extern void* vpkedit_open_with_options([MarshalAs(UnmanagedType.LPStr)] string path, PackFileOptions options);

        [DllImport("libvpkeditc.dll")]
        public static extern PackFileType vpkedit_get_type(void* handle);

        [DllImport("libvpkeditc.dll")]
        public static extern PackFileOptions vpkedit_get_options(void* handle);

        [DllImport("libvpkeditc.dll")]
        public static extern StringArray vpkedit_verify_entry_checksums(void* handle);

        [DllImport("libvpkeditc.dll")]
        public static extern byte vpkedit_verify_file_checksum(void* handle);

        [DllImport("libvpkeditc.dll")]
        public static extern byte vpkedit_is_case_sensitive(void* handle);

        [DllImport("libvpkeditc.dll")]
        public static extern void* vpkedit_find_entry(void* handle, [MarshalAs(UnmanagedType.LPStr)] string filename, byte includeUnbaked);

        [DllImport("libvpkeditc.dll")]
        public static extern Buffer vpkedit_read_entry(void* handle, void* entry);

        [DllImport("libvpkeditc.dll")]
        public static extern String vpkedit_read_entry_text(void* handle, void* entry);

        [DllImport("libvpkeditc.dll")]
        public static extern byte vpkedit_is_read_only(void* handle);

        [DllImport("libvpkeditc.dll")]
        public static extern void vpkedit_add_entry_from_file(void* handle, [MarshalAs(UnmanagedType.LPStr)] string filename, [MarshalAs(UnmanagedType.LPStr)] string pathToFile);

        [DllImport("libvpkeditc.dll")]
        public static extern void vpkedit_add_entry_from_mem(void* handle, [MarshalAs(UnmanagedType.LPStr)] string filename, byte* buffer, ulong bufferLen);

        [DllImport("libvpkeditc.dll")]
        public static extern byte vpkedit_remove_entry(void* handle, [MarshalAs(UnmanagedType.LPStr)] string filename);

        [DllImport("libvpkeditc.dll")]
        public static extern byte vpkedit_bake(void* handle, [MarshalAs(UnmanagedType.LPStr)] string outputDir);

        [DllImport("libvpkeditc.dll")]
        public static extern EntryHandleArray vpkedit_get_baked_entries(void* handle);

        [DllImport("libvpkeditc.dll")]
        public static extern EntryHandleArray vpkedit_get_unbaked_entries(void* handle);

        [DllImport("libvpkeditc.dll")]
        public static extern ulong vpkedit_get_entry_count(void* handle, byte includeUnbaked);

        [DllImport("libvpkeditc.dll")]
        public static extern ulong vpkedit_get_filepath(void* handle, sbyte* buffer, ulong bufferLen);

        [DllImport("libvpkeditc.dll")]
        public static extern ulong vpkedit_get_truncated_filepath(void* handle, sbyte* buffer, ulong bufferLen);

        [DllImport("libvpkeditc.dll")]
        public static extern ulong vpkedit_get_filename(void* handle, sbyte* buffer, ulong bufferLen);

        [DllImport("libvpkeditc.dll")]
        public static extern ulong vpkedit_get_truncated_filename(void* handle, sbyte* buffer, ulong bufferLen);

        [DllImport("libvpkeditc.dll")]
        public static extern ulong vpkedit_get_filestem(void* handle, sbyte* buffer, ulong bufferLen);

        [DllImport("libvpkeditc.dll")]
        public static extern ulong vpkedit_get_truncated_filestem(void* handle, sbyte* buffer, ulong bufferLen);

        [DllImport("libvpkeditc.dll")]
        public static extern ulong vpkedit_get_supported_entry_attributes(void* handle, Attribute* buffer, ulong bufferLen);

        [DllImport("libvpkeditc.dll")]
        public static extern ulong vpkedit_to_string(void* handle, sbyte* buffer, ulong bufferLen);

        [DllImport("libvpkeditc.dll")]
        public static extern void vpkedit_close(void** handle);

        [DllImport("libvpkeditc.dll")]
        public static extern StringArray vpkedit_get_supported_file_types();
    }

    public class PackFile
    {
        private protected unsafe PackFile(void* handle)
        {
            this.handle = handle;
        }

        ~PackFile()
        {
            unsafe
            {
                fixed (void** handlePtr = &handle)
                {
                    Extern.vpkedit_close(handlePtr);
                }
            }
        }

        public static PackFile? Open(string path)
        {
            unsafe
            {
                void* handle = Extern.vpkedit_open(path);
                if (handle == null)
                {
                    return null;
                }
                return new PackFile(handle);
            }
        }

        public static PackFile? Open(string path, PackFileOptions options)
        {
            unsafe
            {
                void* handle = Extern.vpkedit_open_with_options(path, options);
                if (handle == null)
                {
                    return null;
                }
                return new PackFile(handle);
            }
        }

        public static List<string> GetSupportedFileTypes()
        {
            unsafe
            {
                var stringArray = Extern.vpkedit_get_supported_file_types();
                return StringUtils.ConvertToListAndDelete(ref stringArray);
            }
        }

        public List<string> VerifyEntryChecksums()
        {
            unsafe
            {
                var stringArray = Extern.vpkedit_verify_entry_checksums(handle);
                return StringUtils.ConvertToListAndDelete(ref stringArray);
            }
        }

        public bool VerifyFileChecksum()
        {
            unsafe
            {
                return Convert.ToBoolean(Extern.vpkedit_verify_file_checksum(handle));
            }
        }

        public Entry? FindEntry(string filename, bool includeUnbaked = true)
        {
            unsafe
            {
                void* entry = Extern.vpkedit_find_entry(handle, filename, Convert.ToByte(includeUnbaked));
                if (entry == null)
                {
                    return null;
                }
                return new Entry(entry, false);
            }
        }

        public byte[]? ReadEntry(Entry entry)
        {
            unsafe
            {
                var buffer = Extern.vpkedit_read_entry(handle, entry.Handle);
                if (buffer.size < 0)
                {
                    return null;
                }
                return BufferUtils.ConvertToArrayAndDelete(ref buffer);
            }
        }

        public string? ReadEntryText(Entry entry)
        {
            unsafe
            {
                var str = Extern.vpkedit_read_entry_text(handle, entry.Handle);
                if (str.size < 0)
                {
                    return null;
                }
                return StringUtils.ConvertToStringAndDelete(ref str);
            }
        }

        public void AddEntry(string filename, string pathToFile)
        {
            unsafe
            {
                Extern.vpkedit_add_entry_from_file(handle, filename, pathToFile);
            }
        }

        public void AddEntry(string filename, byte[] buffer)
        {
            unsafe
            {
                fixed (byte* bufferPtr = buffer)
                {
                    Extern.vpkedit_add_entry_from_mem(handle, filename, bufferPtr, (ulong) buffer.LongLength);
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
                    Extern.vpkedit_add_entry_from_mem(handle, filename, bufferPtr, (ulong) data.LongLength);
                }
            }
        }

        public bool RemoveEntry(string filename)
        {
            unsafe
            {
                return Convert.ToBoolean(Extern.vpkedit_remove_entry(handle, filename));
            }
        }

        public bool Bake(string outputDir)
        {
            unsafe
            {
                return Convert.ToBoolean(Extern.vpkedit_bake(handle, outputDir));
            }
        }

        public EntryEnumerable GetBakedEntries()
        {
            unsafe
            {
                return new EntryEnumerable(Extern.vpkedit_get_baked_entries(handle));
            }
        }

        public EntryEnumerable GetUnbakedEntries()
        {
            unsafe
            {
                return new EntryEnumerable(Extern.vpkedit_get_unbaked_entries(handle));
            }
        }

        public ulong GetEntryCount(bool includeUnbaked = true)
        {
            unsafe
            {
                return Extern.vpkedit_get_entry_count(handle, Convert.ToByte(includeUnbaked));
            }
        }

        public override string ToString()
        {
            unsafe
            {
                sbyte* stringPtr = stackalloc sbyte[Constants.MAX_PACK_FILE_STRING];
                Extern.vpkedit_to_string(handle, stringPtr, Convert.ToUInt64(Constants.MAX_PACK_FILE_STRING));
                return new string(stringPtr);
            }
        }

        public PackFileType Type
        {
            get
            {
                unsafe
                {
                    return Extern.vpkedit_get_type(handle);
                }
            }
        }

        public PackFileOptions Options
        {
            get
            {
                unsafe
                {
                    return Extern.vpkedit_get_options(handle);
                }
            }
        }

        public bool CaseSensitive
        {
            get
            {
                unsafe
                {
                    return Convert.ToBoolean(Extern.vpkedit_is_case_sensitive(handle));
                }
            }
        }

        public bool ReadOnly
        {
            get
            {
                unsafe
                {
                    return Convert.ToBoolean(Extern.vpkedit_is_read_only(handle));
                }
            }
        }

        public string FilePath
        {
            get
            {
                Span<sbyte> stringArray = new sbyte[Constants.MAX_PATH];
                unsafe
                {
                    fixed (sbyte* stringPtr = stringArray)
                    {
                        Extern.vpkedit_get_filepath(handle, stringPtr, Convert.ToUInt64(stringArray.Length));
                        return new string(stringPtr);
                    }
                }
            }
        }

        public string TruncatedFilePath
        {
            get
            {
                Span<sbyte> stringArray = new sbyte[Constants.MAX_PATH];
                unsafe
                {
                    fixed (sbyte* stringPtr = stringArray)
                    {
                        Extern.vpkedit_get_truncated_filepath(handle, stringPtr, Convert.ToUInt64(stringArray.Length));
                        return new string(stringPtr);
                    }
                }
            }
        }

        public string FileName
        {
            get
            {
                Span<sbyte> stringArray = new sbyte[Constants.MAX_FILENAME];
                unsafe
                {
                    fixed (sbyte* stringPtr = stringArray)
                    {
                        Extern.vpkedit_get_filename(handle, stringPtr, Convert.ToUInt64(stringArray.Length));
                        return new string(stringPtr);
                    }
                }
            }
        }

        public string TruncatedFileName
        {
            get
            {
                Span<sbyte> stringArray = new sbyte[Constants.MAX_FILENAME];
                unsafe
                {
                    fixed (sbyte* stringPtr = stringArray)
                    {
                        Extern.vpkedit_get_truncated_filename(handle, stringPtr, Convert.ToUInt64(stringArray.Length));
                        return new string(stringPtr);
                    }
                }
            }
        }

        public string FileStem
        {
            get
            {
                Span<sbyte> stringArray = new sbyte[Constants.MAX_FILENAME];
                unsafe
                {
                    fixed (sbyte* stringPtr = stringArray)
                    {
                        Extern.vpkedit_get_filestem(handle, stringPtr, Convert.ToUInt64(stringArray.Length));
                        return new string(stringPtr);
                    }
                }
            }
        }

        public string TruncatedFileStem
        {
            get
            {
                Span<sbyte> stringArray = new sbyte[Constants.MAX_FILENAME];
                unsafe
                {
                    fixed (sbyte* stringPtr = stringArray)
                    {
                        Extern.vpkedit_get_truncated_filestem(handle, stringPtr, Convert.ToUInt64(stringArray.Length));
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
                    Attribute* attrArray = stackalloc Attribute[(int) Attribute.COUNT];
                    var numAttributes = Extern.vpkedit_get_supported_entry_attributes(handle, attrArray, (ulong) Attribute.COUNT);

                    var result = new List<Attribute>();
                    for (ulong i = 0; i < numAttributes; i++)
                    {
                        result.Add(attrArray[i]);
                    }
                    return result;
                }
            }
        }

        private protected unsafe readonly void* handle;
    }
}
