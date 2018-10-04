/*
* main.c - a basic chainloader for linux EFI stub kernels
* Copyright (C) 2018 Daniel DeGrasse <daniel@degrasse.com>
* This program is free software: you can redistribute it and/or modify
* it under the terms of the GNU General Public License as published by
* the Free Software Foundation, either version 3 of the License, or
* (at your option) any later version.
* 
* This program is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
* GNU General Public License for more details.
* 
* You should have received a copy of the GNU General Public License
* along with this program.  If not, see <https://www.gnu.org/licenses/>.
*/

#include <efi.h>
#include <efilib.h>
EFI_STATUS
EFIAPI
efi_main (EFI_HANDLE ImageHandle, EFI_SYSTEM_TABLE *SystemTable) {
    InitializeLib(ImageHandle, SystemTable);
    CHAR16 *efi_file = L"\\vmlinuz-linux";
    CHAR16 *load_options=L"initrd=/initramfs-linux.img initrd=/intel-ucode.img cryptdevice=UUID=bd595533-df38-49c6-bac7-76d4b496fb98:cryptroot root=UUID=eb7a10ab-6501-4ebd-9978-2dc3f6195abc rw rootflags=rw,relatime quiet loglevel=3";
    EFI_GUID gloadedimageprotocol = EFI_LOADED_IMAGE_PROTOCOL_GUID;
    EFI_LOADED_IMAGE_PROTOCOL *loadedImage;
    //query the handle to make sure the image protocol is supported
    //this will get use a pointer to the loaded image protocol
    EFI_BOOT_SERVICES *bootservices = SystemTable->BootServices;
    EFI_STATUS status = uefi_call_wrapper(bootservices->HandleProtocol,3,
                                                    ImageHandle,
                                                    &gloadedimageprotocol,
                                                    (void**)&loadedImage);
    switch(status){
        case EFI_SUCCESS:
            //Print(L"Loaded image protocol!\n");
            break;
        case EFI_UNSUPPORTED:
            Print(L"Handle does not support protocol\n");
            return status;
        case EFI_INVALID_PARAMETER:
            Print(L"Invalid Parameter\n");
            return status;
    }

    //now we will convert the handle this image is loaded on into a
    //device path, and use that to make a new device path

    EFI_HANDLE esphandle = loadedImage->DeviceHandle;
    //now we need to get a device path for this handle
    EFI_GUID gdevicepathproto = EFI_DEVICE_PATH_PROTOCOL_GUID;
    EFI_DEVICE_PATH_PROTOCOL *rootpathproto;
    status = uefi_call_wrapper(bootservices->HandleProtocol,3,
                                esphandle,
                                &gdevicepathproto,
                                (void**)&rootpathproto);
    switch(status){
        case EFI_UNSUPPORTED:
            Print(L"did not find a device path for this handle\n");
            return status;
        case EFI_SUCCESS:
            //Print(L"got a device path for root\n");
            break;
    }
    //now use the simple easy gnu efi function to make a full device path
    EFI_DEVICE_PATH *imagetoloadpath = FileDevicePath(esphandle, efi_file);
    EFI_HANDLE imgtoloadhandle;
    status = uefi_call_wrapper(bootservices->LoadImage,6,
                                FALSE,
                                ImageHandle,
                                imagetoloadpath,
                                NULL,
                                0,
                                &imgtoloadhandle);
    switch(status){
        case EFI_SUCCESS:
            //Print(L"Image found and loaded!\n");
            break;
        case EFI_NOT_FOUND:
            Print(L"Image was not able to be located\n");
            return status;
        case EFI_INVALID_PARAMETER:
            Print(L"Invalid parameter supplied\n");
            return status;
        case EFI_LOAD_ERROR:
            Print(L"Image format was not understood\n");
            return status;
        case EFI_SECURITY_VIOLATION:
            Print(L"Security Violation, signature invalid\n");
            return status;
        default:
            Print(L"Unhandled error, status %d\n",status);
            return status;
    }
    //now we need to add boot arguments to our image
    EFI_LOADED_IMAGE_PROTOCOL *loadedimageproto;
    status = uefi_call_wrapper(bootservices->HandleProtocol,3,
                                imgtoloadhandle,
                                &gloadedimageprotocol,
                                (void**)&loadedimageproto);
    switch(status){
        case EFI_SUCCESS:
            //Print(L"Got the loaded image protocol\n");
            break;
        case EFI_UNSUPPORTED:
            Print(L"Device does not support this protocol\n");
            return status;
    }
    //now change the image to have an argument
    loadedimageproto->LoadOptions = load_options;
    UINTN stringlen = StrSize(load_options);
    loadedimageproto->LoadOptionsSize = stringlen;
    

    //now we have a loaded image, hopefully.
    //not printing anything for max speed
    //Print(L"Loading Next EFI Image\n");
    status = uefi_call_wrapper(bootservices->StartImage,3,
                                imgtoloadhandle,
                                0,
                                NULL);
    switch(status){
        case EFI_INVALID_PARAMETER:
            Print(L"The image you want to start does not appear to be loaded\n");
            return status;
        default:
            Print(L"Image did load and exit successfully\n");
            return status;
    }
    //here we need to free any allocated memory we are responsible for
    //should just be device path
    status = uefi_call_wrapper(bootservices->FreePool,1,(void*)imagetoloadpath);
    switch(status){
        case EFI_SUCCESS:
            Print(L"Memory Deallocated successfully\n");
            break;
        case EFI_INVALID_PARAMETER:
            Print(L"Buffer to dealloc was invalid\n");
            return status;
    }
    return EFI_SUCCESS;
}

