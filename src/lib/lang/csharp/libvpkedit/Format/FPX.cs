﻿using System;
using System.Collections.Generic;
using System.Linq;
using System.Runtime.InteropServices;

namespace libvpkedit.Format
{
    internal static unsafe partial class Extern
    {
        [DllImport("libvpkeditc")]
        public static extern void* vpkedit_fpx_open([MarshalAs(UnmanagedType.LPStr)] string path);

        [DllImport("libvpkeditc")]
        public static extern void* vpkedit_fpx_open_with_options([MarshalAs(UnmanagedType.LPStr)] string path, PackFileOptions options);

        [DllImport("libvpkeditc")]
        public static extern byte vpkedit_fpx_generate_keypair_files([MarshalAs(UnmanagedType.LPStr)] string path);

        [DllImport("libvpkeditc")]
        public static extern byte vpkedit_fpx_sign_from_file(void* handle, [MarshalAs(UnmanagedType.LPStr)] string filepath);

        [DllImport("libvpkeditc")]
        public static extern byte vpkedit_fpx_sign_from_mem(void* handle, byte* privateKeyBuffer, ulong privateKeyLen, byte* publicKeyBuffer, ulong publicKeyLen);
    }

    public class FPX : PackFile
    {
        private protected unsafe FPX(void* handle) : base(handle) {}

        public new static FPX? Open(string path)
        {
            unsafe
            {
                var handle = Extern.vpkedit_fpx_open(path);
                return handle == null ? null : new FPX(handle);
            }
        }

        public new static FPX? Open(string path, PackFileOptions options)
        {
            unsafe
            {
                var handle = Extern.vpkedit_fpx_open_with_options(path, options);
                return handle == null ? null : new FPX(handle);
            }
        }

        public static bool GenerateKeyPairFiles(string path)
        {
            unsafe
            {
                return Convert.ToBoolean(Extern.vpkedit_vpk_generate_keypair_files(path));
            }
        }

        public bool Sign(string filepath)
        {
            unsafe
            {
                return Convert.ToBoolean(Extern.vpkedit_fpx_sign_from_file(Handle, filepath));
            }
        }

        public bool Sign(byte[] privateKey, byte[] publicKey)
        {
            unsafe
            {
                fixed (byte* privateKeyBufferPtr = privateKey)
                {
                    fixed (byte* publicKeyBufferPtr = publicKey)
                    {
                        return Convert.ToBoolean(Extern.vpkedit_fpx_sign_from_mem(Handle, privateKeyBufferPtr, (ulong)privateKey.LongLength, publicKeyBufferPtr, (ulong)publicKey.LongLength));
                    }
                }
            }
        }

        public bool Sign(IEnumerable<byte> privateKey, IEnumerable<byte> publicKey)
        {
            var privateKeyData = privateKey.ToArray();
            var publicKeyData = publicKey.ToArray();
            unsafe
            {
                fixed (byte* privateKeyBufferPtr = privateKeyData)
                {
                    fixed (byte* publicKeyBufferPtr = publicKeyData)
                    {
                        return Convert.ToBoolean(Extern.vpkedit_fpx_sign_from_mem(Handle, privateKeyBufferPtr, (ulong)privateKeyData.LongLength, publicKeyBufferPtr, (ulong)publicKeyData.LongLength));
                    }
                }
            }
        }
    }
}
