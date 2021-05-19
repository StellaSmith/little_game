print "Initializing client side"

local my_tbl = setmetatable({}, {__tostring = function(self) return "owo" end})

print("my_tbl:", my_tbl)
