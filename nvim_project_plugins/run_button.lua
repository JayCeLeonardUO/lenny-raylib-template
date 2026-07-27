-- Clickable buttons pinned to the editor's top-right: one-line floating
-- windows over everything, laid out right-to-left in the order given.
-- Clicking one focuses its float, and the BufEnter handler immediately hops
-- back and fires that button's action -- so any click anywhere on a chip
-- acts as a button press (needs 'mouse' on; the keymaps stay the keyboard
-- path to the same actions).

local M = {}

function M.setup(opts)
  if #vim.api.nvim_list_uis() == 0 then return end -- headless: no screen, no buttons

  -- The .nvim.lua auto-reload re-runs setup on every save: keep the one row
  local wins = vim.g.lenny_button_wins
  if wins and #wins > 0 then
    local all_valid = true
    for _, w in ipairs(wins) do
      if not vim.api.nvim_win_is_valid(w) then all_valid = false end
    end
    if all_valid then return end
  end

  local new_wins = {}
  local offset = 0
  for _, btn in ipairs(opts.buttons) do
    local buf = vim.api.nvim_create_buf(false, true)
    vim.api.nvim_buf_set_lines(buf, 0, -1, false, { btn.label })
    vim.bo[buf].modifiable = false
    vim.bo[buf].bufhidden = "hide"

    vim.api.nvim_set_hl(0, btn.hl, { fg = btn.fg, bg = btn.bg, bold = true })

    local width = vim.fn.strdisplaywidth(btn.label)
    local this_offset = offset
    local win = vim.api.nvim_open_win(buf, false, {
      relative = "editor",
      anchor = "NE",
      row = 0,
      col = vim.o.columns - this_offset,
      width = width,
      height = 1,
      style = "minimal",
      zindex = 250,
      focusable = true,
    })
    vim.wo[win].winhl = "Normal:" .. btn.hl
    table.insert(new_wins, win)

    -- Stay glued to the corner when the terminal resizes
    vim.api.nvim_create_autocmd("VimResized", {
      callback = function()
        if vim.api.nvim_win_is_valid(win) then
          vim.api.nvim_win_set_config(win, {
            relative = "editor",
            anchor = "NE",
            row = 0,
            col = vim.o.columns - this_offset,
          })
        end
      end,
    })

    -- The press: entering the button's buffer (a mouse click is the only way
    -- in) hops straight back to where you were and runs
    vim.api.nvim_create_autocmd("BufEnter", {
      buffer = buf,
      callback = function()
        vim.schedule(function()
          vim.cmd.wincmd("p")
          btn.on_click()
        end)
      end,
    })

    offset = offset + width + 1
  end
  vim.g.lenny_button_wins = new_wins
end

return M
