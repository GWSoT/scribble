.686P
.model flat, syscall

public @atomic_inc8@4
public @atomic_inc16@4
public @atomic_inc32@4

public @atomic_dec8@4
public @atomic_dec16@4
public @atomic_dec32@4

public @atomic_add8@8
public @atomic_add16@8
public @atomic_add32@8

public @atomic_sub8@8
public @atomic_sub16@8
public @atomic_sub32@8

public @atomic_swap8@8
public @atomic_swap16@8
public @atomic_swap32@8

public @atomic_cas8@12
public @atomic_cas16@12
public @atomic_cas32@12

.code

@atomic_inc8@4:
    mov dl, 1
    lock xadd byte ptr [ecx], dl
    mov al, dl
    ret

@atomic_inc16@4:
    mov dx, 1
    lock xadd word ptr [ecx], dx
    mov ax, dx
    ret

@atomic_inc32@4:
    mov edx, 1
    lock xadd dword ptr [ecx], edx
    mov eax, edx
    ret


@atomic_dec8@4:
    mov dl, -1
    lock xadd byte ptr [ecx], dl
    mov al, dl
    ret

@atomic_dec16@4:
    mov dx, -1
    lock xadd word ptr [ecx], dx
    mov ax, dx
    ret

@atomic_dec32@4:
    mov edx, -1
    lock xadd dword ptr [ecx], edx
    mov eax, edx
    ret


@atomic_add8@8:
    lock xadd byte ptr [ecx], dl
    mov al, dl
    ret

@atomic_add16@8:
    lock xadd word ptr [ecx], dx
    mov ax, dx
    ret

@atomic_add32@8:
    lock xadd dword ptr [ecx], edx
    mov eax, edx
    ret



@atomic_sub8@8:
    xor al, al
    sub al, dl
    lock xadd byte ptr [ecx], al
    ret

@atomic_sub16@8:
    xor ax, ax
    sub ax, dx
    lock xadd word ptr [ecx], ax
    ret

@atomic_sub32@8:
    xor eax, eax
    sub eax, edx
    lock xadd dword ptr [ecx], eax
    ret


@atomic_swap8@8:
    lock xchg byte ptr [ecx], dl
    mov al, dl
    ret

@atomic_swap16@8:
    lock xchg word ptr [ecx], dx
    mov ax, dx
    ret

@atomic_swap32@8:
    lock xchg dword ptr [ecx], edx
    mov eax, edx
    ret


@atomic_cas8@12:
    mov bl, byte ptr [esp+4]
    mov al, dl
    lock cmpxchg byte ptr [ecx], bl
    ret

@atomic_cas16@12:
    mov bx, word ptr [esp+4]
    mov ax, dx
    lock cmpxchg word ptr [ecx], bx
    ret

@atomic_cas32@12:
    mov ebx, dword ptr [esp+4]
    mov eax, edx
    lock cmpxchg dword ptr [ecx], ebx
    ret

end
