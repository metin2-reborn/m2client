# Development tools; they link the game library like the client does.
function(m2_add_tool name)
    add_executable(${name} ${name}.cpp)
    target_link_libraries(${name} PRIVATE metin2)
    target_compile_options(${name} PRIVATE ${M2_CLEAN_WARNINGS})
    target_link_options(${name} PRIVATE ${M2_EXE_LINK_OPTIONS})
    set_target_properties(${name} PROPERTIES RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin")
endfunction()

# grn_dump: loads every .gr2 of the game data through the Granny runtime (asset validation).
m2_add_tool(grn_dump)
# proto_convert: converts item_proto/mob_proto to format v2 (UTF-8 names).
m2_add_tool(proto_convert)
# spt_stats: generates the trees of .spt files and prints the geometry sizes.
m2_add_tool(spt_stats)
