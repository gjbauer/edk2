/** @file
  UEFI application to scrape contents of RAM.

  Copyright (c) 2025, Gabriel Bauer. All rights reserved.<BR>
  MIT License

**/

#include <Uefi.h>
#include <Library/UefiLib.h>
#include <Library/UefiBootServicesTableLib.h>
#include <Library/MemoryAllocationLib.h>
#include <Library/PrintLib.h>
#include <Protocol/SimpleFileSystem.h>
#include <Protocol/LoadedImage.h>
#include <Library/BaseMemoryLib.h>
#include <Library/BaseLib.h>

VOID
FormatSize (
  IN  UINT64  SizeInBytes,
  OUT CHAR16  *Buffer,
  IN  UINTN   BufferSize
  )
{
  if (SizeInBytes >= (1ULL << 30)) {
    UnicodeSPrint (Buffer, BufferSize, L"%ld GB (%ld bytes)",
                   SizeInBytes >> 30, SizeInBytes);
  } else if (SizeInBytes >= (1ULL << 20)) {
    UnicodeSPrint (Buffer, BufferSize, L"%ld MB (%ld bytes)",
                   SizeInBytes >> 20, SizeInBytes);
  } else if (SizeInBytes >= (1ULL << 10)) {
    UnicodeSPrint (Buffer, BufferSize, L"%ld KB (%ld bytes)",
                   SizeInBytes >> 10, SizeInBytes);
  } else {
    UnicodeSPrint (Buffer, BufferSize, L"%ld bytes", SizeInBytes);
  }
}

/**
  Write data from a buffer to a file on the filesystem.

  @param[in] ImageHandle     Handle of the loaded image
  @param[in] FileName        Name of the file to create/write
  @param[in] Buffer          Buffer containing data to write
  @param[in] BufferSize      Size of the buffer in bytes

  @retval EFI_SUCCESS        File written successfully
  @retval Other              Error occurred during file operations
**/
EFI_STATUS
WriteBufferToFile (
  IN EFI_HANDLE  ImageHandle,
  IN CHAR16      *FileName,
  IN VOID        *Buffer,
  IN UINTN       BufferSize
  )
{
  EFI_STATUS                        Status;
  EFI_LOADED_IMAGE_PROTOCOL         *LoadedImage;
  EFI_SIMPLE_FILE_SYSTEM_PROTOCOL   *FileSystem;
  EFI_FILE_PROTOCOL                 *Root;
  EFI_FILE_PROTOCOL                 *File;
  UINTN                             WriteSize;

  //
  // Get the loaded image protocol to access the device handle
  //
  Status = gBS->HandleProtocol (
                  ImageHandle,
                  &gEfiLoadedImageProtocolGuid,
                  (VOID **)&LoadedImage
                  );
  if (EFI_ERROR (Status)) {
    Print (L"Error: Failed to get LoadedImage protocol (Status = %r)\n", Status);
    return Status;
  }

  //
  // Get the Simple File System protocol from the device handle
  //
  Status = gBS->HandleProtocol (
                  LoadedImage->DeviceHandle,
                  &gEfiSimpleFileSystemProtocolGuid,
                  (VOID **)&FileSystem
                  );
  if (EFI_ERROR (Status)) {
    Print (L"Error: Failed to get SimpleFileSystem protocol (Status = %r)\n", Status);
    return Status;
  }

  //
  // Open the root directory
  //
  Status = FileSystem->OpenVolume (FileSystem, &Root);
  if (EFI_ERROR (Status)) {
    Print (L"Error: Failed to open root volume (Status = %r)\n", Status);
    return Status;
  }

  //
  // Create/Open the file for writing
  //
  Status = Root->Open (
                   Root,
                   &File,
                   FileName,
                   EFI_FILE_MODE_READ | EFI_FILE_MODE_WRITE | EFI_FILE_MODE_CREATE,
                   0
                   );
  if (EFI_ERROR (Status)) {
    Print (L"Error: Failed to create/open file '%s' (Status = %r)\n", FileName, Status);
    Root->Close (Root);
    return Status;
  }

  // Seek to the end of the file
  // (UINT64)-1 is the command to set position to the end of the file
  Status = File->SetPosition(File, (UINT64)-1);
  if (EFI_ERROR(Status)) {
      File->Close(File);
      return Status;
  }

  //
  // Write the buffer to the file
  //
  WriteSize = BufferSize;
  Status = File->Write (File, &WriteSize, Buffer);
  if (EFI_ERROR (Status)) {
    Print (L"Error: Failed to write to file (Status = %r)\n", Status);
  } else if (WriteSize != BufferSize) {
    Print (L"Warning: Only wrote %ld of %ld bytes\n", WriteSize, BufferSize);
    Status = EFI_DEVICE_ERROR;
  }

  File->Flush(File);

  //
  // Close the file and root directory
  //
  File->Close (File);
  Root->Close (Root);

  return EFI_SUCCESS;
}

/**
  Find the largest continuous block of conventional memory.

  @param[out] LargestBase    Base address of the largest block
  @param[out] LargestSize    Size of the largest block in bytes

  @retval EFI_SUCCESS        Successfully found the largest block
  @retval EFI_NOT_FOUND      No conventional memory found
  @retval Other              Error getting memory map
**/
EFI_STATUS
FindLargestConventionalMemoryBlock (
  OUT EFI_PHYSICAL_ADDRESS  *LargestBase,
  OUT UINT64                *LargestSize
  )
{
  EFI_STATUS                Status;
  UINTN                     MemoryMapSize;
  EFI_MEMORY_DESCRIPTOR     *MemoryMap;
  UINTN                     MapKey;
  UINTN                     DescriptorSize;
  UINT32                    DescriptorVersion;
  EFI_MEMORY_DESCRIPTOR     *Descriptor;
  UINTN                     Index;
  UINT64                    CurrentSize;
  EFI_PHYSICAL_ADDRESS      CurrentBase;

  *LargestBase = 0;
  *LargestSize = 0;

  //
  // Get memory map size
  //
  MemoryMapSize = 0;
  Status = gBS->GetMemoryMap (
                  &MemoryMapSize,
                  NULL,
                  &MapKey,
                  &DescriptorSize,
                  &DescriptorVersion
                  );
  if (Status != EFI_BUFFER_TOO_SMALL) {
    return Status;
  }

  //
  // Allocate buffer for memory map (add extra space for potential changes)
  //
  MemoryMapSize += 2 * DescriptorSize;
  MemoryMap = AllocatePool (MemoryMapSize);
  if (MemoryMap == NULL) {
    return EFI_OUT_OF_RESOURCES;
  }

  //
  // Get the actual memory map
  //
  Status = gBS->GetMemoryMap (
                  &MemoryMapSize,
                  MemoryMap,
                  &MapKey,
                  &DescriptorSize,
                  &DescriptorVersion
                  );
  if (EFI_ERROR (Status)) {
    FreePool (MemoryMap);
    return Status;
  }

  //
  // Parse memory map to find largest conventional memory block
  //
  Descriptor = MemoryMap;
  for (Index = 0; Index < MemoryMapSize / DescriptorSize; Index++) {
    if (Descriptor->Type == EfiConventionalMemory) {
      CurrentBase = Descriptor->PhysicalStart;
      CurrentSize = EFI_PAGES_TO_SIZE (Descriptor->NumberOfPages);

      if (CurrentSize > *LargestSize) {
        *LargestBase = CurrentBase;
        *LargestSize = CurrentSize;
      }
    }

    Descriptor = (EFI_MEMORY_DESCRIPTOR *)((UINT8 *)Descriptor + DescriptorSize);
  }

  FreePool (MemoryMap);

  return Status;
}

/**
  The user Entry Point for Application. The user code starts with this function
  as the real entry point for the application.

  @param[in] ImageHandle    The firmware allocated handle for the EFI image.
  @param[in] SystemTable    A pointer to the EFI System Table.

  @retval EFI_SUCCESS       The entry point is executed successfully.
  @retval other             Some error occurs when executing this entry point.

**/
EFI_STATUS
EFIAPI
UefiMain (
  IN EFI_HANDLE        ImageHandle,
  IN EFI_SYSTEM_TABLE  *SystemTable
  )
{
  EFI_STATUS            Status;
  EFI_PHYSICAL_ADDRESS  LargestBase;
  UINT64                LargestSize;
  CHAR16                SizeString[100];

  Print (L"\nUEFI Memory Scraper\n");
  Print (L"================================\n\n");

  //
  // Find the largest continuous block of conventional memory
  //
  Status = FindLargestConventionalMemoryBlock (&LargestBase, &LargestSize);
  if (EFI_ERROR (Status)) {
    if (Status == EFI_NOT_FOUND) {
      Print (L"Error: No conventional memory blocks found!\n");
    } else {
      Print (L"Error: Failed to get memory map (Status = %r)\n", Status);
    }
    return Status;
  }

  //
  // Format and display the results
  //
  FormatSize (LargestSize, SizeString, sizeof (SizeString));

  Print (L"Largest continuous conventional memory block:\n");
  Print (L"  Base Address: 0x%016lx\n", LargestBase);
  Print (L"  Size:         %s\n", SizeString);
  Print (L"  End Address:  0x%016lx\n", LargestBase + LargestSize - 1);

  UINT32 r_size = LargestSize;
  UINT32 w_size = 0;
  UINT8 buffer[262144];
  volatile UINT32 *MemoryPtr = (volatile UINT32 *) (UINTN) LargestBase;
  UINT32 gb_mul_four = LargestSize / ( 4 * 1024 );
  gb_mul_four /= (1024 * 1024);
  UINT32 mb = LargestSize / (1024 * 1024);
  if (mb % ( 4 * 1024 )) gb_mul_four++;
  UINT64 pos = 0;
  UINT32 num = 0;
  UINT32 switch_l = LargestSize / gb_mul_four;
  CHAR8 fname[5];
  CHAR8 fnum;
  CHAR16 fname16[5];

  SetMem ((VOID *)fname, 5, 0);
  while ( r_size > 0 )
  {
    SetMem ((VOID *)buffer, 4096, 0);
    if ( !(pos % switch_l) && pos > 0 ) {
      num++;
      fnum = (CHAR8) ('0' + num);
      SetMem ((VOID *)fname, 5, 0);
      AsciiStrCatS(fname, 1, &fnum );
      AsciiStrCatS(fname, 4, ".bin");
      for (int i=0; i<5; i++)
      {
        fname16[i] = (CHAR16) fname[i];
      }
    }
    if ( r_size >= 262144 ) {
      w_size = 262144;
      r_size -= w_size;
      pos += w_size;
    } else {
      w_size = r_size;
      r_size -= w_size;
      pos += w_size;
    }
    CopyMem (buffer, (VOID*)MemoryPtr, w_size);
    Print (L"%3ld%% through memory dump...\r", (100 * pos) / LargestSize );
    WriteBufferToFile (ImageHandle, fname16, buffer, w_size);
  }

  Print(L"\n");

  Print (L"\nPress any key to exit...\n");

  //
  // Wait for user input before exiting
  //
  SystemTable->ConIn->Reset (SystemTable->ConIn, FALSE);
  EFI_INPUT_KEY Key;
  while (SystemTable->ConIn->ReadKeyStroke (SystemTable->ConIn, &Key) == EFI_NOT_READY) {
    // Wait for key press
  }

  return EFI_SUCCESS;
}
