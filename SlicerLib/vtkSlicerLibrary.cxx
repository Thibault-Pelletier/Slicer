#include "vtkSlicerLibrary.h"

#include <vtkObjectFactory.h>

vtkStandardNewMacro(vtkSlicerLibrary);

vtkSlicerLibrary::vtkSlicerLibrary() = default;
vtkSlicerLibrary::~vtkSlicerLibrary() = default;

int vtkSlicerLibrary::GetVersionMajor()
{
    return 5;
}

int vtkSlicerLibrary::GetVersionMinor()
{
    return 7;
}

int vtkSlicerLibrary::GetVersionPatch()
{
    return 0;
}

const char* vtkSlicerLibrary::GetVersionString()
{
    return "5.7.0";
}
