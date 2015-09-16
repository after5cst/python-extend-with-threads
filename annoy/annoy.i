%module annoy
%{
 #include "annoy.h"
 extern void init_globals();
%}
#include "annoy.h"

%init %{
    init_globals();
%}
