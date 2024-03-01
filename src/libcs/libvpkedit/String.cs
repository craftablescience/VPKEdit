using System.Collections.Generic;
using System.Runtime.InteropServices;

namespace libvpkedit
{
    internal unsafe static partial class Extern
    {
        [DllImport("libvpkeditc.dll")]
        public static extern String vpkedit_string_new(ulong size);

        [DllImport("libvpkeditc.dll")]
        public static extern void vpkedit_string_free(String* str);

        [DllImport("libvpkeditc.dll")]
        public static extern StringArray vpkedit_string_array_new(ulong size);

        [DllImport("libvpkeditc.dll")]
        public static extern void vpkedit_string_array_free(StringArray* array);
    }

    [StructLayout(LayoutKind.Sequential)]
    internal struct String
    {
        public ulong size;
        public unsafe sbyte* data;
    }

    [StructLayout(LayoutKind.Sequential)]
    internal struct StringArray
    {
        public ulong size;
        public unsafe sbyte** data;
    }

    internal unsafe static class StringUtils
    {
        public static string ConvertToStringAndDelete(ref String str)
        {
            var result = new string(str.data);

            fixed (String* strPtr = &str)
            {
                Extern.vpkedit_string_free(strPtr);
            }

            return result;
        }

        public static List<string> ConvertToListAndDelete(ref StringArray array)
        {
            var strings = new List<string>();

            for (ulong i = 0; i < array.size; i++)
            {
                strings.Add(new string(array.data[i]));
            }

            fixed (StringArray* arrayPtr = &array)
            {
                Extern.vpkedit_string_array_free(arrayPtr);
            }

            return strings;
        }
    }
}
