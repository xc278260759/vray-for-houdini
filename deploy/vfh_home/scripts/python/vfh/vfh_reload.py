#
# Copyright (c) 2015-2018, Chaos Software Ltd
#
# V-Ray For Houdini
#
# ACCESSIBLE SOURCE CODE WITHOUT DISTRIBUTION OF MODIFICATION LICENSE
#
# Full license text: https://github.com/ChaosGroup/vray-for-houdini/blob/master/LICENSE
#

def reload_module(m):
    import os

    HOUDINI_SOHO_DEVELOPER = os.environ.get("HOUDINI_SOHO_DEVELOPER", False)
    if not HOUDINI_SOHO_DEVELOPER:
        return

    print "Reloading: %s" % (m.__file__)
    reload(m)
