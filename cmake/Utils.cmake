# Provides an option that the user can enable/disable.
# Positional arguments:
#   variable : variable where to store user choice
#   description : help string describing the option
#   value : initial value or boolean expression
#
# ~ Simplified version of corresponding OCV_OPTION macro from OpenCV
macro(RADICAL_OPTION variable description value)
  set(__value ${value})
  if(__value MATCHES ";")
    if(${__value})
      option(${variable} "${description}" ON)
    else()
      option(${variable} "${description}" OFF)
    endif()
  elseif(DEFINED ${__value})
    if(${__value})
      option(${variable} "${description}" ON)
    else()
      option(${variable} "${description}" OFF)
    endif()
  else()
    option(${variable} "${description}" ${__value})
  endif()
  unset(__value)
endmacro()

