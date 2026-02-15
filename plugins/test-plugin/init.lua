local M = {}

M.name = "test-plugin"
M.version = "1.0.0"

function M.on_load()
  aetherius.log.info("test-plugin loaded")
  aetherius.include("server/hello.lua")
end

function M.on_unload()
  aetherius.log.info("test-plugin unloaded")
end

aetherius.hook_add("server_start", "test_plugin_start", function()
  aetherius.log.info("server_start event")
end)

aetherius.hook_add("player_join", "test_plugin_join", function(player)
  player:send_message("Welcome, " .. player:get_name() .. "!")
end)

aetherius.hook_add("player_leave", "test_plugin_leave", function(player, reason)
  aetherius.log.info("Player left: " .. player:get_name() .. " reason=" .. reason)
end)

aetherius.hook_add("player_chat", "test_plugin_chat", function(player, message)
  if message == "ping" then
    player:send_message("pong")
    return false
  end
  return message
end)

aetherius.register_command("luaping", "Lua test command", function(player, args)
  if player then
    player:send_message("Lua command ok")
  end
end)

return M
