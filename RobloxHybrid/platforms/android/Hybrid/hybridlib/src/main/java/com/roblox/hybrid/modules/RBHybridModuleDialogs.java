package com.roblox.hybrid.modules;

import com.roblox.hybrid.RBHybridCommand;
import com.roblox.hybrid.RBHybridModule;

/**
 * Created by roblox on 3/6/15.
 */
public class RBHybridModuleDialogs extends RBHybridModule {
    private static final String MODULE_ID = "Dialogs";

    public RBHybridModuleDialogs() {
        super(MODULE_ID);
    }

    @Override
    public void execute(RBHybridCommand command) {
        command.executeCallback(false, null);
    }
}
