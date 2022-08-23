#!lua

local function df_probe (opt, disk)
	local str, errno
	str, errno = df.cmd("probe", "--" .. opt, disk)
	if (errno ~= 0) then
		return ""
	end
	return string.sub(str, 0, -2)
end

local function df_name_disk (disk, num)
	if (df_probe("rm", disk) == "REMOVABLE") then
		return "RM" .. num
	end
	return "DISK" .. num
end

local function df_get_label (label)
	if (label == nil or label == "") then
		return ""
	end
	return "[" .. label .. "]"
end

local function df_get_fs (fs)
	if (fs) then
		return string.upper(fs)
	end
	return "UNKNOWN"
end

local function df_get_flag (disk, partmap)
	local flag = df_probe("flag", disk)
	if (partmap == "msdos") then
		if (string.find(flag, "ACTIVE")) then
			return "激活"
		end
	else
		local attr = string.match(flag, "^[%x%-]+ ([%a%s]+)$", 1)
		if (attr) then
			return attr
		end
	end
	return ""
end 

local function df_enum_iter (disk, fs, uuid, label, size)
	local num
	if (disk == "proc") then
		return 0
	end
	num = string.match(disk, "^%ld(%d+)$", 1)
	if (num) then
		print(string.format("%-6s %-10s %s %s",
			df_name_disk(disk, num),
			size,
			df_probe("bus", disk),
			df_probe("pid", disk)))
	else
		partmap, num = string.match(disk, "^%ld%d+,(%a+)(%d+)$", 1)
		print(string.format("|- PART%d %-10s %-10s %s %s",
			num,
			size,
			df_get_fs(fs),
			df_get_flag(disk, partmap),
			df_get_label(label)))
	end
end

df.enum_device(df_enum_iter)
