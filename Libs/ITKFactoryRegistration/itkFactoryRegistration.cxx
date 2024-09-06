#include "itkFactoryRegistration.h"
#include "itkFactoryRegistrationInternals.h"

// The following code is required to ensure that the
// mechanism allowing the ITK factory to be registered is not
// optimized out by the compiler.
void itk::itkFactoryRegistration()
{
  itkFactoryRegistrationInternals();
}
