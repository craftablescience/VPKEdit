using System;
using System.Collections;
using System.Collections.Generic;
using System.Runtime.InteropServices;

namespace libvpkedit
{
    internal unsafe static partial class Extern
    {
        [DllImport("libvpkeditc.dll")]
        public static extern ulong vpkedit_entry_get_path(void* handle, sbyte* buffer, ulong bufferLen);

        [DllImport("libvpkeditc.dll")]
        public static extern ulong vpkedit_entry_get_parent_path(void* handle, sbyte* buffer, ulong bufferLen);

        [DllImport("libvpkeditc.dll")]
        public static extern ulong vpkedit_entry_get_filename(void* handle, sbyte* buffer, ulong bufferLen);

        [DllImport("libvpkeditc.dll")]
        public static extern ulong vpkedit_entry_get_stem(void* handle, sbyte* buffer, ulong bufferLen);

        [DllImport("libvpkeditc.dll")]
        public static extern ulong vpkedit_entry_get_extension(void* handle, sbyte* buffer, ulong bufferLen);

        [DllImport("libvpkeditc.dll")]
        public static extern void vpkedit_entry_free(void** handle);

        [DllImport("libvpkeditc.dll")]
        public static extern void vpkedit_entry_array_free(EntryHandleArray* array);
    }

    [StructLayout(LayoutKind.Sequential)]
    internal unsafe struct EntryHandleArray
    {
        internal ulong size;
        internal void** data;
    }

    public class Entry
    {
        unsafe internal Entry(void* handle, bool inArray)
        {
            Handle = handle;
            this.inArray = inArray;
        }

        ~Entry()
        {
            if (!inArray)
            {
                unsafe
                {
                    fixed (void** handlePtr = &Handle)
                    {
                        Extern.vpkedit_entry_free(handlePtr);
                    }
                }
            }
        }

        public string Path
        {
            get
            {
                Span<sbyte> stringArray = new sbyte[Constants.MAX_PATH];
                unsafe
                {
                    fixed (sbyte* stringPtr = stringArray)
                    {
                        Extern.vpkedit_entry_get_path(Handle, stringPtr, Convert.ToUInt64(stringArray.Length));
                        return new string(stringPtr);
                    }
                }
            }
        }

        public string ParentPath
        {
            get
            {
                Span<sbyte> stringArray = new sbyte[Constants.MAX_PATH];
                unsafe
                {
                    fixed (sbyte* stringPtr = stringArray)
                    {
                        Extern.vpkedit_entry_get_parent_path(Handle, stringPtr, Convert.ToUInt64(stringArray.Length));
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
                        Extern.vpkedit_entry_get_filename(Handle, stringPtr, Convert.ToUInt64(stringArray.Length));
                        return new string(stringPtr);
                    }
                }
            }
        }

        public string Stem
        {
            get
            {
                Span<sbyte> stringArray = new sbyte[Constants.MAX_FILENAME];
                unsafe
                {
                    fixed (sbyte* stringPtr = stringArray)
                    {
                        Extern.vpkedit_entry_get_stem(Handle, stringPtr, Convert.ToUInt64(stringArray.Length));
                        return new string(stringPtr);
                    }
                }
            }
        }

        public string Extension
        {
            get
            {
                Span<sbyte> stringArray = new sbyte[Constants.MAX_FILENAME];
                unsafe
                {
                    fixed (sbyte* stringPtr = stringArray)
                    {
                        Extern.vpkedit_entry_get_extension(Handle, stringPtr, Convert.ToUInt64(stringArray.Length));
                        return new string(stringPtr);
                    }
                }
            }
        }

        internal unsafe void* Handle;

        private readonly bool inArray;
    }

    public class EntryEnumerable : IEnumerable<Entry>
    {
        internal unsafe EntryEnumerable(EntryHandleArray array)
        {
            this.array = array;
        }

        ~EntryEnumerable()
        {
            unsafe
            {
                fixed (EntryHandleArray* arrayPtr = &array)
                {
                    Extern.vpkedit_entry_array_free(arrayPtr);
                }
            }
        }

        private Entry GetEntryAtPosition(ulong pos)
        {
            unsafe
            {
                return new Entry(array.data[pos], true);
            }
        }

        IEnumerator<Entry> GetEnumerator()
        {
            for (ulong i = 0; i < array.size; i++)
            {
                yield return GetEntryAtPosition(i);
            }
        }

        IEnumerator<Entry> IEnumerable<Entry>.GetEnumerator()
        {
            return GetEnumerator();
        }

        IEnumerator IEnumerable.GetEnumerator()
        {
            return GetEnumerator();
        }

        internal EntryHandleArray array;
    }
}
