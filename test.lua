--[[
local ModifyType = { 
    Unmodified = 0,
    Blocked = 1,
    Modified = 2
}

local NetIODirection = {
    Send = 0,
    Receive = 1
}

local ValueType = {
    Unknown = 0,
    Bool = 1,
    Int32 = 2,
    Int64 = 3,
    UInt32 = 4,
    UInt64 = 5,
    Float = 6,
    Double = 7,
    Enum = 8,
    ByteSequence = 9,
    String = 10,
    List = 11,
    Map = 12,
    Node = 13
}
]]

-- Calling when this script put to filter
-- Must return bool, true if packet passed filter, false otherwise
function on_filter(packet)
    return true
end

-- Calling when script enabled in 'Packet modify' window
function on_modify(packet)
    -- Packet:enable_modifying()
    -- For some opeartions which marked with [modification],
    -- needs enabling modification. Otherwise such operation will return error.
    packet:enable_modifying() -- Enabling modifying

    -- Packet:name()
    -- Return: string -- typename of packet
    -- Note. The same result as by packet:content():node():type()
	print("Name:", packet:name()) -- PlayerTimeNotify

    -- Packet:mid()
    -- Return: int -- message id
    print("UniqueID:", packet:uid()) -- 972
    
    -- Packet:mid()
    -- Return: int -- message id
    print("MessageID:", packet:mid()) -- 316

    -- Packet:timestamp()
    -- Return: int -- packet receive timestamp in ms
    print("Timestamp:", packet:timestamp()) -- 1657141751375
    
    -- Packet:direction()
    -- Return: int -- io direction, check NetIODirection
    print("Direction:", packet:direction()) -- 0 | NetIODirection.Send

    -- Packet:is_packed()
    -- Return: bool -- is message packed
    -- Note. The result the same as by packet:content():is_packed()
    print("Is packed:", packet:is_packed()) -- false

    -- Packet:content()
    -- Return: Message -- content message
    -- Note. The content message contains all data related to packet type.
    local content = packet:content() -- same structure as in head

    -- Packet:head()
    -- Return: Message -- head message
    -- Note. The head message contains transmission data, such timestamp or sequence id.
    local head = packet:head()
    
    -- Message:has_unknown()
    -- Return: bool -- has unknown fields
    -- Note. For details check Field:is_unknown()
    print("Head|Has unknown:", head:has_unknown()) -- false

    -- Message:has_unsetted()
    -- Return: bool -- has unsetted fields
    -- Note. For details check Field:is_unsetted()
    print("Head|Has unsetted:", head:has_unsetted()) -- false
    
    -- Message:is_packed()
    -- Return: bool -- is message packed
    -- Note. Packed is message what is inside another message.
    --       For example CombatInvocationNotify always is in UnionCmdNotify.
    print("Head|Is packed:", head:is_packed()) -- false

    -- Message:node()
    -- Return: Node -- node structure of message
    local head_node = head:node() 

    -- Node:type()
    -- Return: string -- return node type name
    -- Note. For unknown types will be '<unknown>'.
    print("Head node type:", head_node:type()) -- PacketHead

    -- Node:fields()
    -- Return: table -- array of node's fields
    local fields = head_node:fields()
    for i, field in ipairs(fields) do
        print(i, field:name()) -- check declarartion below
    end

    -- Node:has_field(name or number)
    -- Return: bool -- is field exists
    local exist = head_node:has_field("client_sequence_id")

    -- Node:create_field(number, name, value) [modification]
    -- Creating field in node with specified data.
    -- Note. If number is -1, then field automaticaly will marked as custom.
    head_node:create_field(--[[number: ]] 101, --[[name: ]] "some_field", --[[value: ]] 10)

    -- Node:remove_field(name or number) [modification]
    -- Return: bool -- operation success
    -- Note. If field doesn't exist return false
    head_node:remove_field("some_field") -- or head_node:remove_field(101)

    print("Head|Has client_sequence_id:", exist)    
    if exist then
        -- Node:set_field(name or number, value) [modification]
        -- Return: bool -- is operation successed
        -- Note. Setting field value. Field must exist.
        --       If field doesn't exist returns false.
        print("Set field success:", head_node:set_field("client_sequence_id", 100)) -- true

        -- Node:field(name or number)
        -- Return: Field or nil.
        -- Note. Find field in node by it's name or number.
        --       When not found return nil.
        local csid_field = head_node:field("client_sequence_id") -- or head_node:field(3)
        
        -- Field:number()
        -- Return: number - field number.
        -- Note. Field number it's identifier of each field.
        --       For custom fields number() always -1
        print("Head[client_sequence_id] number:", csid_field:number()) -- 3

        -- Field:name()
        -- Return: string - field name.
        print("Head[client_sequence_id] name:", csid_field:name()) -- client_sequence_id
        
        -- Field:is_unknown()
        -- Return: bool - is field unknown.
        -- Note. Unknown fields - it's kind of fields what don't exist in proto type,
        --       but exist in proto binary input. This fields can appear when proto type is
        --       incorrect or doesn't exist at all.
        --       Such fields has generated name, better identify such fields by it's number.
        print("Head[client_sequence_id] is unknown:", csid_field:is_unknown()) -- false

        -- Field:is_unsetted()
        -- Return: bool - is field unsetted.
        -- Note. Unsetted fields it's fields what wasn't setted in proto binary 
        --       but there is in proto type. This fields will not written to output binary.
        --       Also this status will disappear if you set new value to field.
        print("Head[client_sequence_id] is unsetted:", csid_field:is_unsetted()) -- false

        -- Field:is_custom()
        -- Return: bool - is field custom.
        -- Note. For detail check Field:make_custom()
        print("Head[client_sequence_id] is custom:", csid_field:is_custom()) -- false

        -- Field:make_custom() [modification]
        -- Mark the field as custom.
        -- Custom fields will not written to output packet binary. 
        -- This fields only decoration.
        csid_field:make_custom()

        -- Field:value()
        -- Return: Value.
        -- Note. Value is not lua raw value, it's custom structure.
        local csid_value = csid_field:value()

        -- Value:get()
        -- Return: bool or string or number or table or Node.
        print("Head[client_sequence_id] value:", csid_value:get()) -- 1321
        
        -- Value:type()
        -- Return: int - type of value. 
        -- Note. Check ValueType struct.
        print("Head[client_sequence_id] value type:", csid_value:type()) -- 4 | ValueType.UInt32
        
        -- Value:set(value) [modification]
        -- Setting value content.
        -- Note. Table value not implemented yet.
        csid_value:set(1)
    end

    -- You must to return one of three states
    -- ModifyType.Unmodified - if you don't want to do anything with packet
	-- ModifyType.Blocked - if you want to block send/receive packet
    -- ModifyType.Modified - if you want to apply changes what you do to 'packet'
    return ModifyType.Unmodified
end