using System;
using System.Collections;
using System.Collections.Generic;
using System.Runtime.InteropServices;

namespace libvpkedit
{
    internal static unsafe partial class Extern
    {
        [DllImport("libvpkeditc")]
        public static extern ulong vpkedit_entry_get_path(void* handle, sbyte* buffer, ulong bufferLen);

        [DllImport("libvpkeditc")]
        public static extern ulong vpkedit_entry_get_parent_path(void* handle, sbyte* buffer, ulong bufferLen);

        [DllImport("libvpkeditc")]
        public static extern ulong vpkedit_entry_get_filename(void* handle, sbyte* buffer, ulong bufferLen);

        [DllImport("libvpkeditc")]
        public static extern ulong vpkedit_entry_get_stem(void* handle, sbyte* buffer, ulong bufferLen);

        [DllImport("libvpkeditc")]
        public static extern ulong vpkedit_entry_get_extension(void* handle, sbyte* buffer, ulong bufferLen);

        [DllImport("libvpkeditc")]
        public static extern void vpkedit_entry_free(void** handle);

        [DllImport("libvpkeditc")]
        public static extern void vpkedit_entry_array_free(EntryHandleArray* array);

        [DllImport("libvpkeditc")]
        public static extern ulong vpkedit_virtual_entry_get_name(void* handle, sbyte* buffer, ulong bufferLen);

        [DllImport("libvpkeditc")]
        public static extern byte vpkedit_virtual_entry_is_writable(void* handle);

        [DllImport("libvpkeditc")]
        public static extern void vpkedit_virtual_entry_free(void** handle);

        [DllImport("libvpkeditc")]
        public static extern void vpkedit_virtual_entry_array_free(VirtualEntryHandleArray* array);
    }

    [StructLayout(LayoutKind.Sequential)]
    internal unsafe struct EntryHandleArray
    {
        internal long size;
        internal void** data;
    }

    [StructLayout(LayoutKind.Sequential)]
    internal unsafe struct VirtualEntryHandleArray
    {
        internal long size;
        internal void** data;
    }

    public class Entry
    {
        internal unsafe Entry(void* handle, bool inArray)
        {
            Handle = handle;
            _inArray = inArray;
        }

        ~Entry()
        {
            if (!_inArray)
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
                Span<sbyte> stringArray = new sbyte[Constants.MaxPath];
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
                Span<sbyte> stringArray = new sbyte[Constants.MaxPath];
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
                Span<sbyte> stringArray = new sbyte[Constants.MaxFilename];
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
                Span<sbyte> stringArray = new sbyte[Constants.MaxFilename];
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
                Span<sbyte> stringArray = new sbyte[Constants.MaxFilename];
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

        private readonly bool _inArray;
    }

    public class EntryEnumerable : IEnumerable<Entry>
    {
        internal EntryEnumerable(EntryHandleArray array)
        {
            _array = array;
        }

        ~EntryEnumerable()
        {
            unsafe
            {
                fixed (EntryHandleArray* arrayPtr = &_array)
                {
                    Extern.vpkedit_entry_array_free(arrayPtr);
                }
            }
        }

        private Entry GetEntryAtPosition(ulong pos)
        {
            unsafe
            {
                return new Entry(_array.data[pos], true);
            }
        }

        private IEnumerator<Entry> GetEnumerator()
        {
            for (long i = 0; i < _array.size; i++)
            {
                yield return GetEntryAtPosition((ulong) i);
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

        private EntryHandleArray _array;
    }

    public class VirtualEntry
    {
        internal unsafe VirtualEntry(void* handle, bool inArray)
        {
            Handle = handle;
            _inArray = inArray;
        }

        ~VirtualEntry()
        {
            if (!_inArray)
            {
                unsafe
                {
                    fixed (void** handlePtr = &Handle)
                    {
                        Extern.vpkedit_virtual_entry_free(handlePtr);
                    }
                }
            }
        }

        public string Name
        {
            get
            {
                Span<sbyte> stringArray = new sbyte[Constants.MaxFilename];
                unsafe
                {
                    fixed (sbyte* stringPtr = stringArray)
                    {
                        Extern.vpkedit_virtual_entry_get_name(Handle, stringPtr, Convert.ToUInt64(stringArray.Length));
                        return new string(stringPtr);
                    }
                }
            }
        }
        
        public bool Writable
        {
            get
            {
                unsafe
                {
                    return Convert.ToBoolean(Extern.vpkedit_virtual_entry_is_writable(Handle));
                }
            }
        }

        internal unsafe void* Handle;

        private readonly bool _inArray;
    }

    public class VirtualEntryEnumerable : IEnumerable<VirtualEntry>
    {
        internal VirtualEntryEnumerable(VirtualEntryHandleArray array)
        {
            _array = array;
        }

        ~VirtualEntryEnumerable()
        {
            unsafe
            {
                fixed (VirtualEntryHandleArray* arrayPtr = &_array)
                {
                    Extern.vpkedit_virtual_entry_array_free(arrayPtr);
                }
            }
        }

        private VirtualEntry GetVirtualEntryAtPosition(ulong pos)
        {
            unsafe
            {
                return new VirtualEntry(_array.data[pos], true);
            }
        }

        private IEnumerator<VirtualEntry> GetEnumerator()
        {
            for (long i = 0; i < _array.size; i++)
            {
                yield return GetVirtualEntryAtPosition((ulong) i);
            }
        }

        IEnumerator<VirtualEntry> IEnumerable<VirtualEntry>.GetEnumerator()
        {
            return GetEnumerator();
        }

        IEnumerator IEnumerable.GetEnumerator()
        {
            return GetEnumerator();
        }

        private VirtualEntryHandleArray _array;
    }
}
