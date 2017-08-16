/*
 * LATRDProcessPluginLib.cpp
 *
 *  Created on: 26 Apr 2017
 *      Author: gnx91527
 */

#include "LATRDFileWriterPlugin.h"
#include "ClassLoader.h"

namespace FrameProcessor
{
    /**
     * Registration of this decoder through the ClassLoader.  This macro
     * registers the class without needing to worry about name mangling
     */
    REGISTER(FrameProcessorPlugin, LATRDFileWriterPlugin, "LATRDFileWriterPlugin");

} // namespace FrameReceiver
