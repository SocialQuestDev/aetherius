#include "../../../../include/network/packet/play/UpdateHealthPacket.h"

UpdateHealthPacket::UpdateHealthPacket(float health, int food, float saturation)
    : health(health), food(food), saturation(saturation) {}

void UpdateHealthPacket::write(PacketBuffer& buffer) {
    buffer.writeFloat(health);
    buffer.writeVarInt(food);
    buffer.writeFloat(saturation);
}
