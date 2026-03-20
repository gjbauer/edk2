## @file                                                                                                                                                        
#  Platform description file for building the ScraperPkg UEFI application.                                                                                  
#                                                                                                                                                               
#  Copyright (c) 2025, Gabriel Bauer. All rights reserved.<BR>                                                                                              
#  MIT License                                                                                                               
#                                                                                                                                                               
##                                                                                                                                                              
                                                                                                                                                                
[Defines]                                                                                                                                                       
  PLATFORM_NAME                  = ScraperPkg                                                                                                               
  PLATFORM_GUID                  = 87654321-4321-4321-4321-210987654321                                                                                         
  PLATFORM_VERSION               = 1.0                                                                                                                          
  DSC_SPECIFICATION              = 0x00010005                                                                                                                   
  OUTPUT_DIRECTORY               = Build/ScraperPkg                                                                                                         
  SUPPORTED_ARCHITECTURES        = IA32|X64|AARCH64|RISCV64                                                                                                     
  BUILD_TARGETS                  = DEBUG|RELEASE|NOOPT                                                                                                          
  SKUID_IDENTIFIER               = DEFAULT                                                                                                                      
                                                                                                                                                                
[LibraryClasses]                                                                                                                                                
  UefiApplicationEntryPoint|MdePkg/Library/UefiApplicationEntryPoint/UefiApplicationEntryPoint.inf                                                              
  UefiLib|MdePkg/Library/UefiLib/UefiLib.inf                                                                                                                    
  UefiBootServicesTableLib|MdePkg/Library/UefiBootServicesTableLib/UefiBootServicesTableLib.inf                                                                 
  MemoryAllocationLib|MdePkg/Library/UefiMemoryAllocationLib/UefiMemoryAllocationLib.inf                                                                        
  PrintLib|MdePkg/Library/BasePrintLib/BasePrintLib.inf                                                                                                         
  BaseLib|MdePkg/Library/BaseLib/BaseLib.inf                                                                                                                    
  BaseMemoryLib|MdePkg/Library/BaseMemoryLib/BaseMemoryLib.inf                                                                                                  
  DebugLib|MdePkg/Library/BaseDebugLibNull/BaseDebugLibNull.inf                                                                                                 
  DevicePathLib|MdePkg/Library/UefiDevicePathLib/UefiDevicePathLib.inf                                                                                          
  PcdLib|MdePkg/Library/BasePcdLibNull/BasePcdLibNull.inf                                                                                                       
  UefiRuntimeServicesTableLib|MdePkg/Library/UefiRuntimeServicesTableLib/UefiRuntimeServicesTableLib.inf
  RegisterFilterLib|MdePkg/Library/RegisterFilterLibNull/RegisterFilterLibNull.inf
  StackCheckLib|MdePkg/Library/StackCheckLibNull/StackCheckLibNull.inf
                                                                                                                                                                
[Components]                                                                                                                                                    
  ScraperPkg/ScraperPkg.inf                                                                                                        
                                                                                                                                                                
[BuildOptions]                                                                                                                                                  
  GCC:*_*_*_CC_FLAGS = -DDISABLE_NEW_DEPRECATED_INTERFACES -mcmodel=medany -mno-relax
