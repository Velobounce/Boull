package net.minecraft.client;

import net.minecraft.client.entity.EntityPlayerSP;
import net.minecraft.client.gui.GuiScreen;
import net.minecraft.client.multiplayer.WorldClient;

public class Minecraft {
    public net.minecraft.util.Session session = new net.minecraft.util.Session("Original", "old", "token", "legacy");
    public static Minecraft theMinecraft = new Minecraft();
    public WorldClient theWorld = new WorldClient();
    public EntityPlayerSP thePlayer = new EntityPlayerSP();
    public GuiScreen currentScreen = null;
    public int leftClickCounter = 7;
    private static int debugFPS = 144;
}

