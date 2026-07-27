-- Project-local Neovim config, auto-loaded on launch when `vim.o.exrc = true`
vim.opt.makeprg = "make -C src"

-- CMake build (generates compile_commands.json for LSP/clangd)
vim.api.nvim_create_user_command("CMakeBuild", function()
    vim.cmd("!cmake -B build && cmake --build build")
end, {})

vim.keymap.set("n", "<leader>mm", "<cmd>make<cr>", { desc = "Build (make -C src)" })
vim.keymap.set("n", "<leader>mr", "<cmd>!src/lenny-raylib-template<cr>", { desc = "Run game" })
