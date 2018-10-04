#include <efi.h>
#include <efilib.h>
EFI_STATUS
EFIAPI
efi_main (EFI_HANDLE ImageHandle, EFI_SYSTEM_TABLE *SystemTable) {
    InitializeLib(ImageHandle, SystemTable);
    Print(L"Looking to see what our own loaded image handle looks like\n");
    EFI_GUID loadedimageprotocol = EFI_LOADED_IMAGE_PROTOCOL_GUID;
    EFI_LOADED_IMAGE_PROTOCOL *loadedImage;
    //query the handle to make sure the image protocol is supported
    //this will get use a pointer to the loaded image protocol
    EFI_BOOT_SERVICES *bootservices = SystemTable->BootServices;
    EFI_STATUS status = uefi_call_wrapper(bootservices->HandleProtocol,3,
                                                    ImageHandle,
                                                    &loadedimageprotocol,
                                                    (void**)&loadedImage);
    if (status != EFI_SUCCESS){
        Print(L"Failed to get loaded image handle protocol\n");
        Print(L"Status was %d\n",status);
        return status;
    }
    EFI_DEVICE_PATH_PROTOCOL *filepath = loadedImage->FilePath;
    //here we also get the efi handle this file resides on, because that part of the handle is revelent as well
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
            Print(L"got a device path for root\n");
            break;
    }
    //try to open the efi device path to text protocol, to examine what is in this path
    //this protocol is not installed on most handles, so we have to locate it.
    EFI_DEVICE_PATH_TO_TEXT_PROTOCOL *pathtotextproto;
    EFI_GUID pathtotextguid=EFI_DEVICE_PATH_TO_TEXT_PROTOCOL_GUID;
    status = uefi_call_wrapper(bootservices->LocateProtocol,3,
                                &pathtotextguid,
                                NULL,//registration parameter
                                (void**)&pathtotextproto);
    if (status != EFI_SUCCESS){
        Print(L"Error getting the path to text protocol\n");
        Print(L"Status was %d\n",status);
        return status;
    }else if (pathtotextproto == NULL){
        Print(L"No handle matched for the path to text proto\n");
    }
    //if we make it here, we have successfully found the protocol and we can use it.
    CHAR16 *text_rep = (CHAR16*)uefi_call_wrapper(pathtotextproto->ConvertDevicePathToText,3,
                    filepath,
                    0,
                    0);
    if(text_rep != NULL){
        Print(L"File Path of current executable: %s\n",text_rep);
    }else{
        Print(L"Failed to convert path\n");
    }
    text_rep = (CHAR16*)uefi_call_wrapper(pathtotextproto->ConvertDevicePathToText,3,
                    rootpathproto,
                    0,
                    0);
    if(text_rep != NULL){
        Print(L"File Path of current executable: %s\n",text_rep);
    }else{
        Print(L"Failed to convert path\n");
    }
     
    // now we will try to open the text to handle protocol and load an executable using it.
    EFI_DEVICE_PATH_FROM_TEXT_PROTOCOL *pathfromtextproto;
    EFI_GUID pathfromtextguid = EFI_DEVICE_PATH_FROM_TEXT_PROTOCOL_GUID;
    status=uefi_call_wrapper(bootservices->LocateProtocol,3,
                            &pathfromtextguid,
                            NULL,//registration parameter, unused
                            (void**)&pathfromtextproto);
    if (status != EFI_SUCCESS){
        Print(L"Failed to find path from text protocol! Status:%d \n",status);
        return status;
    }
    //with the text, to handle protocol, we now want to try and open an efi 
    //executable. Lets form a device handle for an executable at the root of our esp 
    CHAR16 *fullpath =  L"PciRoot(0x0)/Pci(0x1,0x1)/Ata(Primary,Master,0x0)/HD(1,MBR,0xBE1AFDFA,0x3F,0xFBFC1)\\Print.efi";
    Print(L"Locating %s\n",fullpath);
    EFI_DEVICE_PATH_PROTOCOL *imgtoloadpath = (EFI_DEVICE_PATH_PROTOCOL*)uefi_call_wrapper(pathfromtextproto->ConvertTextToDevicePath,1,
                                                            fullpath);
    if(imgtoloadpath != NULL){
        Print(L"Device Path Discovered!\n");
    }else{
        Print(L"Device Path Discovery Failed\n");
        return EFI_NOT_FOUND;
    }
    //now try to load the image
    EFI_HANDLE *imagetoloadhandle;
    status = uefi_call_wrapper(bootservices->LoadImage,6,
                                0, //we are the boot manager I guess
                                ImageHandle,
                                imgtoloadpath,
                                NULL,
                                0,
                                &imagetoloadhandle);
    Print(L"made it past load image\n");
    switch(status){
        case EFI_SUCCESS:
            Print(L"Image found and loaded!\n");
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
    return EFI_SUCCESS;
}











